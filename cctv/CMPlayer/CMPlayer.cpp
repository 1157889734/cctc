#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "CMPlayer.h"
#include "../vdec/vdec.h"
#include "../rtsp/rtspApi.h"
#include "../rtsp/rtspComm.h"
#include "../debugout/debug.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include "types.h"


static int g_iConnectNum = 0;

#define PLAY_STREAM_TYPE_PREVIEW  0
#define PLAY_STREAM_TYPE_PLAYBACK 1

typedef struct _T_WORK_PACKET
{
    int iMsgCmd;
    double dValue;
} T_WORK_PACKET, *PT_WORK_PACKET;

typedef struct _T_WORK_PACKET_LIST
{
    T_WORK_PACKET tPkt;
    struct _T_WORK_PACKET_LIST *next;
} T_WORK_PACKET_LIST;

typedef struct _T_WORK_QUEUE
{
    T_WORK_PACKET_LIST *ptFirst, *ptLast;
    int iQueueType;  // 0:先进先出(FIFO)，1:后进先出(LIFO)
    int iPktCount;
    pthread_mutex_t *pMutex;
				
} T_WORK_QUEUE, *PT_WORK_QUEUE;

typedef struct _T_CMP_PLAYER_INFO
{
    char acUrl[256];
    int iMonitorPlayThreadRunFlag;
    int iThreadExitFlag;
    int iPlayState;
    int iPlayStreamType;     // 0: preview, 1: playback
    int iTcpFlag;
    int iPlayRange;
    int iCurrentPlayTime; //当前播放时间，单位毫秒
    int iRtspHeartCount;   
    int iGetFrameFlag;   //获取到帧数据的标识
    int iStreamState; //是否有码流状态，0-无码流，1-有码流
    int iPlaySpeed;
    unsigned int uiPrevFrameTs;
    unsigned int uiPlayBaseTime;
    int iIgnoreFrameNum;
	int iIFrameFlag;
    RTSP_HANDLE RHandle;   //RTSP句柄
    VDEC_HADNDLE VHandle;   //解码句柄
    pthread_mutex_t tMutex;
    pthread_t monitorPlayThreadId;   //RTSP处理线程
    PT_WORK_QUEUE ptQueue;	
	void *pWinHandle;
} T_CMP_PLAYER_INFO,*PT_CMP_PLAYER_INFO;


PT_WORK_QUEUE CreateWorkQueue(pthread_mutex_t *pMutex, int iQueueType)
{
    PT_WORK_QUEUE ptWorkQueue = NULL;
    
    ptWorkQueue = (PT_WORK_QUEUE)malloc(sizeof(T_WORK_QUEUE));
    if (NULL == ptWorkQueue)
    {
        return NULL;
    }
    memset(ptWorkQueue, 0, sizeof(T_WORK_QUEUE));
    ptWorkQueue->pMutex = pMutex;
    ptWorkQueue->iQueueType = iQueueType;
    ptWorkQueue->ptLast = NULL;
    ptWorkQueue->ptFirst = NULL;
    ptWorkQueue->iPktCount= 0;

    return ptWorkQueue;
}

int DestroyWorkQueue(PT_WORK_QUEUE ptWorkQueue)
{
    T_WORK_PACKET_LIST *ptPktList = NULL, *ptTmp;

    if (NULL == ptWorkQueue)
    {
        return -1;
    }

    if (ptWorkQueue->pMutex)
    {
        pthread_mutex_lock(ptWorkQueue->pMutex);
    }

    ptPktList = ptWorkQueue->ptFirst;
    while (ptPktList)
    {
        ptTmp = ptPktList;
        ptPktList = ptPktList->next;
        free(ptTmp);
    }
	
    ptWorkQueue->ptLast = NULL;
    ptWorkQueue->ptFirst = NULL;
    ptWorkQueue->iPktCount= 0;

    if (ptWorkQueue->pMutex)
    {
        pthread_mutex_unlock(ptWorkQueue->pMutex);
    }

    free(ptWorkQueue);
    ptWorkQueue = NULL;

    return 0;
}

int PutNodeToWorkQueue(PT_WORK_QUEUE ptWorkQueue, PT_WORK_PACKET ptPkt)
{
    T_WORK_PACKET_LIST *ptPktList = NULL;


    if ((NULL == ptWorkQueue) || (NULL == ptPkt))
    {
        return -1;
    }
    ptPktList = (T_WORK_PACKET_LIST *)malloc(sizeof(T_WORK_PACKET_LIST));
    if (NULL == ptPktList)
    {
        return -1;
    }

    memset(ptPktList, 0, sizeof(T_WORK_PACKET_LIST));
    ptPktList->tPkt = *ptPkt;

    if (ptWorkQueue->pMutex)
    {
        pthread_mutex_lock(ptWorkQueue->pMutex);
    }

    if (NULL == ptWorkQueue->ptLast)
    {
	    ptWorkQueue->ptFirst = ptPktList;
    }
    else
    {
	    ptWorkQueue->ptLast->next = ptPktList;
    }
    ptWorkQueue->ptLast = ptPktList;
    ptWorkQueue->iPktCount++;

    if (ptWorkQueue->pMutex)
    {
        pthread_mutex_unlock(ptWorkQueue->pMutex);
    }

    return 0;
}

int GetNodeFromWorkQueue(PT_WORK_QUEUE ptWorkQueue, PT_WORK_PACKET ptPkt)
{
    T_WORK_PACKET_LIST *ptTmp = NULL;

    if ((NULL == ptWorkQueue) || (NULL == ptPkt))
    {
        return 0;
    }

    if (ptWorkQueue->pMutex)
    {
        pthread_mutex_lock(ptWorkQueue->pMutex);
    }

    if (NULL == ptWorkQueue->ptFirst)
    {
        if (ptWorkQueue->pMutex)
        {
            pthread_mutex_unlock(ptWorkQueue->pMutex);
        }

        return 0;
    }
	
    ptTmp = ptWorkQueue->ptFirst;
    ptWorkQueue->ptFirst = ptWorkQueue->ptFirst->next;
    if (NULL == ptWorkQueue->ptFirst)
    {
        ptWorkQueue->ptLast= NULL;
    }
    ptWorkQueue->iPktCount--;
    *ptPkt = ptTmp->tPkt;
    free(ptTmp);

    if (ptWorkQueue->pMutex)
    {
        pthread_mutex_unlock(ptWorkQueue->pMutex);
    }
    
    return 1;
}

void SetPlayState(CMPHandle hPlay, int iPlayState)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
        return;
    }

    ptCmpPlayer->iPlayState = iPlayState;
}

static int RtpSetDataCallBack(int iStreamType, char *pcFrame, int iFrameLen, unsigned int uiTs, void *pUserData)
{
    int iRet = 0;
    
    if (STREAM_TYPE_VIDEO == iStreamType)
    {
        PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)pUserData;
        
		ptCmpPlayer->iGetFrameFlag = 1;   //拿到rtp数据，将获取到帧数据的标识置1
		ptCmpPlayer->iRtspHeartCount = 0;   //拿到rtp数据，将计数清0，表示有流了重新进行计数监控
        ptCmpPlayer->iStreamState = 1;   //拿到rtp数据，则将码流状态置1，表示有流了
		
        if ((PLAY_STREAM_TYPE_PLAYBACK == ptCmpPlayer->iPlayStreamType) && (0 == ptCmpPlayer->iIgnoreFrameNum))
        {
            unsigned int uiNpt = 0;
            
            if ((ptCmpPlayer->uiPrevFrameTs != 0) && (uiTs >= ptCmpPlayer->uiPrevFrameTs))
            {
                uiNpt = (uiTs - ptCmpPlayer->uiPrevFrameTs) / 90;
                if (uiNpt > 5000) //(40 * 25 * 5)
                {
                    uiNpt = 40;	
                }
                
            }
            else if (0 == ptCmpPlayer->uiPrevFrameTs)
            {
                uiNpt = 0;
            }
            else
            {
                uiNpt = 40;
            }
            
            if (CMP_STATE_FAST_FORWARD == ptCmpPlayer->iPlayState)
            {
                ptCmpPlayer->iCurrentPlayTime += uiNpt;// * ptCmpPlayer->iPlaySpeed;
            }
            else if (CMP_STATE_SLOW_FORWARD == ptCmpPlayer->iPlayState)
            {
                ptCmpPlayer->iCurrentPlayTime += uiNpt;// / ptCmpPlayer->iPlaySpeed;
            }
            else
            {
                ptCmpPlayer->iCurrentPlayTime += uiNpt;
            }
            
            ptCmpPlayer->uiPrevFrameTs = uiTs;
            
            if (ptCmpPlayer->iCurrentPlayTime / 1000 >= ptCmpPlayer->iPlayRange)
            {
                ptCmpPlayer->iCurrentPlayTime = (ptCmpPlayer->iPlayRange - 1) * 1000;
            }
        }
        else
        {
            ptCmpPlayer->uiPrevFrameTs = uiTs;
        }
        if (ptCmpPlayer->iIgnoreFrameNum > 0)
        {
            ptCmpPlayer->iIgnoreFrameNum --;
        }
        
        VDEC_SendVideoStream(ptCmpPlayer->VHandle, (char *)pcFrame, iFrameLen);
    }
    
    return iRet;
}

int ParseRtspUrl(char *pcRawUrl, char *pcUrl, char *pcUser, char *pcPasswd)
{
    char acStr[256];
    char *pcTmp = NULL;
    char *pcPos = NULL;
    char *pcContent = NULL;
    char *pcIpaddr = NULL;
    int iPos = 0;
    int iRet = 0;
    
    if ((NULL == pcRawUrl) || (NULL == pcUrl) || (NULL == pcUser) || (NULL == pcUser))
    {
        return -1;	
    }
    memset(acStr, 0, sizeof(acStr));
    strncpy(acStr, pcRawUrl, sizeof(acStr));
   
    /* rtsp:// */
    pcContent = acStr + 7;
    pcTmp = strsep(&pcContent, "/");
    
    if (pcTmp)
    {
        pcIpaddr = pcTmp;
        if(strstr(pcIpaddr, "@"))
        {
            pcTmp = strsep(&pcIpaddr, "@");
        }
        else
        {
            pcTmp = NULL;	
        }
    }

    if (pcTmp)
    {
        pcPos = pcTmp;
        pcTmp = strsep(&pcPos, ":");
        
        if (pcTmp)
        {
            strcpy(pcUser, pcTmp);
        }
        
        if (pcPos)
        {
            strcpy(pcPasswd, pcPos);
        }
        
    }
    
    if (NULL == pcIpaddr)
    {
        return -1;	
    }

    if (pcContent)
    {
        snprintf(pcUrl, 256, "rtsp://%s/%s", pcIpaddr, pcContent);
    }
    else
    {
        snprintf(pcUrl, 256, "rtsp://%s", pcIpaddr);
    }
    
    
    return iRet;
}

static void *MonitorPlayThread(void *arg)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)arg;
    RTSP_HANDLE RHandle = 0;
    int iTimeout = 0;
    int iRet = 0;
    char acUrl[256] = {0}, acUserName[64] = {0}, acPassWd[64] = {0};
    int iRunCount = 0;
    int iReconnectFlag = 1;
    int iRtpProtocol = ptCmpPlayer->iTcpFlag;
    T_WORK_PACKET tWorkPkt;
	Fl_Group* pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;  //根据窗口句柄匹配窗体	
    int iFreshCount = 0;
    
    pthread_detach(pthread_self());

    if (NULL == arg)
    {
        return NULL;
    }

    memset(acUrl, 0, sizeof(acUrl));
    memset(acUserName, 0, 64);
    memset(acPassWd, 0, 64);
    ParseRtspUrl(ptCmpPlayer->acUrl, acUrl, acUserName, acPassWd);
    
    while (ptCmpPlayer->iMonitorPlayThreadRunFlag && iReconnectFlag)
    {
        if (RHandle)
        {		
            iRet = RTSP_CloseStream(RHandle, 2);
            iRet = RTSP_Logout(RHandle);
            RHandle = 0;
            RTSP_MSleep(50);
        }

        RHandle = RTSP_Login(acUrl, acUserName, acPassWd);
        if (RHandle > 0)
        {			
            ptCmpPlayer->RHandle = RHandle;
            RTSP_GetParam(RHandle, E_TYPE_PLAY_RANGE, (void *)&ptCmpPlayer->iPlayRange);
            iRet =  RTSP_OpenStream(RHandle, 1, iRtpProtocol, (void *)RtpSetDataCallBack, (void *)ptCmpPlayer);
            if (iRet < 0)
            {	
            	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "RTSP_OpenStream error!iRet=%d\n",iRet);
				
                RTSP_Logout(RHandle);
                RHandle = 0;
				if(0 == iFreshCount)
                {
                    pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                    VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h());
                }
                iFreshCount ++;
                iFreshCount = iFreshCount%20;
                RTSP_MSleep(50);	
                continue;
            }
			
            iRet =  RTSP_PlayControl(RHandle, E_PLAY_STATE_PLAY, 0);
            if (iRet < 0)
            {
                if(0 == iFreshCount)
                {
                    pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                    VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h()); 
                }
                iFreshCount ++;
                iFreshCount = iFreshCount%20;
           		continue;
            }
			ptCmpPlayer->iIFrameFlag = 1;
            SetPlayState(ptCmpPlayer, CMP_STATE_PLAY);
			
            RTSP_MSleep(50);
			
            iTimeout = RTSP_GetRtspTimeout(RHandle);
            if (0 == iTimeout)
            {
                iTimeout = 60;
            }
            iFreshCount = 0;
        }
        else
        {		
            DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "rtsp_login %s error!\n", ptCmpPlayer->acUrl);  
            if (PLAY_STREAM_TYPE_PLAYBACK == ptCmpPlayer->iPlayStreamType)     //回放的时候不让重连
            {
            	iReconnectFlag = 0;
            }
            if(0 == iFreshCount)
            {
                 pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                 VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h());
            }
            iFreshCount ++;
            iFreshCount = iFreshCount%20;
            RTSP_MSleep(50);
            continue;
        }

        while (ptCmpPlayer->iMonitorPlayThreadRunFlag)
        {	
            iRet = GetNodeFromWorkQueue(ptCmpPlayer->ptQueue, &tWorkPkt);
            if (iRet > 0)
            {
                DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "MonitorPlayThread get playctrl cmd:%d, value=%lf\n", tWorkPkt.iMsgCmd, tWorkPkt.dValue);
                iRet = RTSP_PlayControl(RHandle, tWorkPkt.iMsgCmd, tWorkPkt.dValue);
                if (iRet < 0)
                {				
                    DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "MonitorPlayThread  RTSP_PlayControl error! err=%d\n",iRet);
                }				
                RTSP_MSleep(50);
            }

			ptCmpPlayer->iRtspHeartCount++;

			if (PLAY_STREAM_TYPE_PREVIEW == ptCmpPlayer->iPlayStreamType) //实时预览连续2s及大于2s，码流数据回调没拿到数据而不能让iCountNum清0，则表示此时的码流状态为无流
			{
				if (ptCmpPlayer->iRtspHeartCount >= 40)   
                {   
                    pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                    VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h());
                    ptCmpPlayer->iStreamState = 0;
                    ptCmpPlayer->iRtspHeartCount = 0;
                    break;
                }
			}
			else  //回放(非暂停状态)分两种情况: 1、统计的当前播放时间还没有接近总时长(30s为界定)，则还是以5s为参考来判断无流。2、当前播放时间接近总时长，则缩短到1s为参考来判断，使回放不会因为进度不准而不能及时关闭
			{	
				if (ptCmpPlayer->iPlayState != CMP_STATE_PAUSE)   //非暂停状态
				{
					if ((ptCmpPlayer->iCurrentPlayTime >= (ptCmpPlayer->iPlayRange - 30)*1000) && (ptCmpPlayer->iCurrentPlayTime <= ptCmpPlayer->iPlayRange*1000))
					{
						if (ptCmpPlayer->iRtspHeartCount >= 20)
						{
							pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                            VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h());
                            
                            ptCmpPlayer->iStreamState = 0;
							iReconnectFlag = 0;
							break;
						}
					}
					else
					{
						if (ptCmpPlayer->iRtspHeartCount >= 100)
						{
							pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                            VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h());
                            
                            ptCmpPlayer->iStreamState = 0;
							iReconnectFlag = 0;
							break;
						}
					}
				}
			}
			
            if (0 == iRunCount)
            {
                iRunCount = iTimeout * 2;
                iRet = RTSP_SendHeart(RHandle);
                if (iRet < 0)
                {
                    DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "send heart failed!\n");
                    if (PLAY_STREAM_TYPE_PLAYBACK == ptCmpPlayer->iPlayStreamType)  //回放的时候不让重连
                    {
                        iReconnectFlag = 0;
                    }
					pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;
                    VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,pPlayWin->x(),pPlayWin->y(),pPlayWin->w(),pPlayWin->h());
                   
                    ptCmpPlayer->iStreamState = 0;
                    break;
                }
            }
            iRunCount--;
					
            RTSP_MSleep(50);
        }
    }	

	if (PLAY_STREAM_TYPE_PLAYBACK == ptCmpPlayer->iPlayStreamType)
	{
	    ptCmpPlayer->iCurrentPlayTime = (ptCmpPlayer->iPlayRange) * 1000;   //
	}

	 //线程循环主体退出，置标志为1，使CMP_CloseMedia函数能尽快返回，因为下面这个RTSP_Logout会有所耗时。线程一开始就设置了退出自动回收资源，所以这样也不会造成内存泄漏
    RTSP_CBFunEnable(RHandle, 0);
    RTSP_MSleep(1);
    ptCmpPlayer->iThreadExitFlag = 1;  
    if (RHandle)
    {
        iRet = RTSP_CloseStream(RHandle, 2);
        iRet = RTSP_Logout(RHandle);
        RHandle = 0;
    }

    return NULL;
}

int CmpRootFsType(char *pcFsType)
{
    FILE *pFile = NULL;
    char acBuf[128];
    char acFileName[]   = "/proc/mounts";

    pFile = fopen(acFileName, "rb");
    if (NULL == pFile)
    {
        return -1;
    }

    memset(acBuf, 0x0, sizeof(acBuf));
    while (fgets(acBuf, sizeof(acBuf), pFile))
    {
        if (strstr(acBuf, "/dev/root") != NULL) 
        {
            fclose(pFile);
            if (strstr(acBuf, pcFsType) != NULL)
            {
                return 0;
            }
            return -1;
        }
        memset(acBuf, 0x0, sizeof(acBuf));
    }
    
    fclose(pFile);
    
    return -1;
}


int GetHardwareAuthResult(void)
{
    int iRet = 0;
    
    iRet = access("/sbin/start.sh", F_OK);
    if (iRet<0)
    {
        return -1;
    }
   
    iRet = CmpRootFsType("cramfs");
    
    return iRet;
}


/*************************************************
  函数功能:     CMP_Init
  函数描述:     模块初使化
  输入参数:     无
  输出参数:     无
  返回值:       0-成功，<0-失败
  作者：        
  日期:         
*************************************************/
int CMP_Init(PT_VIDEO_INFO ptVdecInfo)
{
    int iRet = 0, i = 0;
    int iBackgroud = 0x000001, iAlpha=0xff;
    pthread_mutexattr_t	mutexattr;

	if(GetHardwareAuthResult()<0)
	{
		return -1;
	}
	
    signal(SIGPIPE,SIG_IGN);    
    iRet = VDEC_Init(ptVdecInfo->iScreenWidth, ptVdecInfo->iScreenHeight);
    if (iRet < 0)
    {
        return CMP_VDECINIT_ERR;
    }
    iRet = VDEC_SetGuiBackground(iBackgroud);    //设置gui背景颜色
    if (iRet < 0)
    {
        return CMP_VDECINIT_ERR;
    }
    iRet = VDEC_SetGuiAlpha(iAlpha);   //设置GUI透明度为不透明
    if (iRet < 0)
    {
        return CMP_VDECINIT_ERR;
    }
	iRet =  VDEC_Setfb1BKColor(NULL,0,0,ptVdecInfo->iScreenWidth,ptVdecInfo->iScreenHeight);
	if (iRet < 0)
    {
        return CMP_VDECINIT_ERR;
    }	
    return 0;
}

/*************************************************
  函数功能:     CMP_UnInit
  函数描述:     模块反初使化
  输入参数:     无
  输出参数:     无
  返回值:       0：成功， <0:失败
  作者：    
  日期:         
  修改:
*************************************************/
int CMP_UnInit(void)
{
    VDEC_UnInit();
	return 0;
}

int CMP_SetBlackBackground(int ix,int iy,int iWidth,int iHeight)
{
	return VDEC_Setfb1BKColor(NULL,ix,iy,iWidth,iHeight);
}

/*************************************************
  函数功能:     CMP_CreateMedia
  函数描述:     创建媒体句柄
  输入参数:     tWnd：媒体显示窗口位置信息
                iVolumeEnable:使能声音
  返回值:       媒体句柄
  作者：        
  日期:         2016-11-01
*************************************************/
CMPHandle CMP_CreateMedia(void *pWinHandle)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = NULL;
    int iRet = 0;
    int iWinX = 0;
    int iWinY = 0;
    int iWinWidth = 1024;
    int iWinHeight = 768;
    pthread_mutexattr_t	mutexattr;

    ptCmpPlayer = (PT_CMP_PLAYER_INFO)malloc(sizeof(T_CMP_PLAYER_INFO));
    if (NULL == ptCmpPlayer)
    {
        return NULL;
    }
    memset(ptCmpPlayer, 0, sizeof(T_CMP_PLAYER_INFO));
		
    iRet = VDEC_CreateVideoDecCh();
    if (0 == iRet)
    {
        DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_CreateMedia error! VDEC_CreateVideoDecCh error!iRet = %d\n",iRet);
        free(ptCmpPlayer);
        ptCmpPlayer = NULL;
        return NULL;
    }
    ptCmpPlayer->VHandle = (VDEC_HADNDLE)iRet;
	ptCmpPlayer->pWinHandle = pWinHandle;
	
	Fl_Group* pPlayWin=(Fl_Group *)pWinHandle;  //根据窗口句柄匹配窗体
    if (NULL == pPlayWin)
    {
        free(ptCmpPlayer);
        ptCmpPlayer = NULL;
        return NULL;
    }
    
    iWinX = pPlayWin->x();
    iWinY = pPlayWin->y();
    iWinWidth = pPlayWin->w();
    iWinHeight = pPlayWin->h();
	
    iRet = VDEC_SetDecWindowsPos(ptCmpPlayer->VHandle, iWinX, iWinY, iWinWidth, iWinHeight);
    if (iRet < 0)
    {
        DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_CreateMedia error! VDEC_SetDecWindowsPos error!iRet = %d\n",iRet);
        free(ptCmpPlayer);
        ptCmpPlayer = NULL;
        return NULL;
    }
  
    iRet = VDEC_VideoDecDisp(ptCmpPlayer->VHandle,0,0);
    if (iRet < 0)
    {
        DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_CreateMedia error! VDEC_DisableVideoDecCh error!iRet = %d\n",iRet);
        free(ptCmpPlayer);
        ptCmpPlayer = NULL;
        return NULL;
    }
	

    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_TIMED_NP);
    pthread_mutex_init(&ptCmpPlayer->tMutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    ptCmpPlayer->ptQueue = CreateWorkQueue(&ptCmpPlayer->tMutex, 0);
    g_iConnectNum ++;
    DebugPrint(DEBUG_ERROR_PRINT,"Camer Open Num = %d\n",g_iConnectNum);
    return (CMPHandle)ptCmpPlayer;
}

/*************************************************
  函数功能:     CMP_DestroyMedia
  函数描述:     销毁媒体句柄
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:       0：成功， <0:失败
  作者：  
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_DestroyMedia(CMPHandle hPlay)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
	
    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_DestroyMedia error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

	
    VDEC_DestroyVideoDecCh(ptCmpPlayer->VHandle);
	
	//Fl_Group* pPlayWin = (Fl_Group*)ptCmpPlayer->pWinHandle;  //根据窗口句柄匹配窗体
	//pPlayWin->color(FL_BLACK);
	//pPlayWin->redraw();
	
    DestroyWorkQueue(ptCmpPlayer->ptQueue);
    pthread_mutex_destroy(&ptCmpPlayer->tMutex); 

	if (ptCmpPlayer != NULL)
    {
        free(ptCmpPlayer);
        ptCmpPlayer = NULL;
    }
    g_iConnectNum --;
    DebugPrint(DEBUG_ERROR_PRINT, "Camer Close Num = %d\n",g_iConnectNum);
    return 0;
}

/*************************************************
  函数功能:     CMP_SetWndDisplayEnable
  函数描述:     设置播放窗口显示使能
  输入参数:     hPlay：媒体句柄
                iEnable: 使能标志，1-使能，0-不使能
  输出参数:     无
  返回值:
  作者：    
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_SetWndDisplayEnable(CMPHandle hPlay, int iDispEnable,int iDecEnable)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
    int iRet = 0;
	int iTmpDispEnable = 0;
    int iTmpDecEnable = 0;
    
    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_SetWndDisplayEnable error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }
	VDEC_GetVideoDecDispEnable(ptCmpPlayer->VHandle,&iTmpDecEnable,&iTmpDispEnable);
    
	if((iDispEnable == iTmpDispEnable) && (iDecEnable == iTmpDecEnable))
	{
    	return 0;
    }
    if (iDispEnable)
    {
    	Fl_Group* pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;  //根据窗口句柄匹配窗体

		pPlayWin->color(0x00000100);  
		pPlayWin->redraw();
        iRet = VDEC_VideoDecDisp(ptCmpPlayer->VHandle,1,1);
        if (iRet < 0)
        {
			DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_SetWndDisplayEnable error! VDEC_EnableVideoDecCh error! err=%d\n",iRet);
            return CMP_VDECCH_ENABLE_ERR;
        }
    }
    else
    {   	
        iRet = VDEC_VideoDecDisp(ptCmpPlayer->VHandle,0,iDecEnable);
		//Fl_Group* pPlayWin=(Fl_Group *)ptCmpPlayer->pWinHandle;  //根据窗口句柄匹配窗体	
		//pPlayWin->color(FL_BLACK);
		//pPlayWin->redraw();
        if (iRet < 0)
        {
			DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_SetWndDisplayEnable error! VDEC_DisableVideoDecCh error! err=%d\n",iRet);
            return CMP_VDECCH_ENABLE_ERR;
        }
    }
	
	DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "CMP_SetWndDisplayEnable Ok!\n");
    return 0;
}



/*************************************************
  函数功能:     CMP_ChangeWnd
  函数描述:     改变播放窗口
  输入参数:     hPlay：媒体句柄 tWnd 改变后的目标窗口
  输出参数:     无
  返回值:
  作者：       
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_ChangeWnd(CMPHandle hPlay, void *pWinHandle)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;	
    int iRet = 0;
    int iWinX = 0;
    int iWinY = 0;
    int iWinWidth = 1024;
    int iWinHeight = 768;
	int iEnableShow = 0;
    int iDecEnable = 0;
    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_ChangeWnd error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

	Fl_Group* pPlayWin =  (Fl_Group*)pWinHandle;;  //根据窗口句柄匹配窗体
	iWinX = pPlayWin->x();
    iWinY = pPlayWin->y();
    iWinWidth = pPlayWin->w();
    iWinHeight = pPlayWin->h();
	
	VDEC_GetVideoDecDispEnable(ptCmpPlayer->VHandle,&iDecEnable,&iEnableShow);
	if(1 == iEnableShow && ptCmpPlayer->pWinHandle != pWinHandle)
	{
		pPlayWin = (Fl_Group*)ptCmpPlayer->pWinHandle;
//		pPlayWin->color(FL_BLACK);  
//		pPlayWin->redraw();

		pPlayWin = (Fl_Group*)pWinHandle;
		pPlayWin->color(0x00000100);  
		pPlayWin->redraw();
		ptCmpPlayer->pWinHandle = pWinHandle;
	}
	if (0 == ptCmpPlayer->iStreamState && iEnableShow)   /*流断了进行窗口切换时，重新对当前通道进行一下使能显示，以免出现断流后窗口切换过程中总显示最后一帧静止图像问题*/
	{
		VDEC_Setfb1BKColor(ptCmpPlayer->VHandle,iWinX, iWinY, iWinWidth, iWinHeight);
	}
    iRet = VDEC_SetDecWindowsPos(ptCmpPlayer->VHandle, iWinX, iWinY, iWinWidth, iWinHeight);
    if (iRet < 0)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_ChangeWnd error! VDEC_SetDecWindowsPos error, err=%d\n", iRet);
        return CMP_WND_SETPOS_ERR;
    }
	
    return 0;
}

/*************************************************
  函数功能:     CMP_OpenMediaPreview
  函数描述:     打开预览媒体
  输入参数:     hPlay：媒体句柄， pcRtspUrl：rtsp地址， iTcpFlag：CMP_TCP or CMP_UDP
  输出参数:     无
  返回值:       0：成功， -1:未找到相应媒体句柄
  作者：      
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_OpenMediaPreview(CMPHandle hPlay, const char *pcRtspUrl, int iTcpFlag)
{
	int iRet = 0;
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_OpenMediaPreview error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    ptCmpPlayer->iPlayStreamType = PLAY_STREAM_TYPE_PREVIEW;
    ptCmpPlayer->monitorPlayThreadId = 0;
    sprintf(ptCmpPlayer->acUrl,  "%s", pcRtspUrl);
    ptCmpPlayer->iMonitorPlayThreadRunFlag = 1;
    ptCmpPlayer->iTcpFlag = iTcpFlag;
	ptCmpPlayer->iRtspHeartCount = 0;
	ptCmpPlayer->iGetFrameFlag = 0;
	ptCmpPlayer->iStreamState = 0;    //打开时默认为还没有码流
	ptCmpPlayer->iThreadExitFlag = 0;
	
    DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "CMP_OpenMediaPreview rtspUrl=%s\n", pcRtspUrl);
	iRet = VDEC_StartVideoDec(ptCmpPlayer->VHandle);
	if (iRet < 0)
	{
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_OpenMediaPreview error! VDEC_StartVideoDec error, err=%d\n", iRet);
	}
    pthread_create(&ptCmpPlayer->monitorPlayThreadId, NULL, MonitorPlayThread, (void *)ptCmpPlayer);    //创建rtsp处理线程
    SetPlayState(ptCmpPlayer, CMP_STATE_IDLE);	

    return 0;
}

/*************************************************
  函数功能:     CMP_OpenMediaFile
  函数描述:     打开点播媒体
  输入参数:     hPlay：媒体句柄， pcRtspFile:rtsp文件地址 ， iTcpFlag：CMP_TCP or CMP_UDP
  输出参数:     无
  返回值:       0：成功， -1:未找到相应媒体句柄
  作者：        
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_OpenMediaFile(CMPHandle hPlay, const char *pcRtspFile, int iTcpFlag)
{
	int iRet = 0;
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_OpenMediaFile error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    ptCmpPlayer->iPlayStreamType = PLAY_STREAM_TYPE_PLAYBACK;
    ptCmpPlayer->monitorPlayThreadId = 0;
    sprintf(ptCmpPlayer->acUrl,  "%s", pcRtspFile);
    ptCmpPlayer->iMonitorPlayThreadRunFlag = 1;
    ptCmpPlayer->iTcpFlag = iTcpFlag;
	ptCmpPlayer->iPlaySpeed = 1;
	ptCmpPlayer->iRtspHeartCount = 0;
	ptCmpPlayer->iThreadExitFlag = 0;
	ptCmpPlayer->iCurrentPlayTime = 0;
	
	iRet = VDEC_StartVideoDec(ptCmpPlayer->VHandle);
	if (iRet < 0)
	{
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_OpenMediaFile error! VDEC_StartVideoDec error, err=%d\n", iRet);
	}
	
    pthread_create(&ptCmpPlayer->monitorPlayThreadId, NULL, MonitorPlayThread, (void *)ptCmpPlayer);
    SetPlayState(ptCmpPlayer, CMP_STATE_IDLE);

    return 0;
}

/*************************************************
  函数功能:     CMP_CloseMedia
  函数描述:     关闭媒体
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:       0：成功， -1:未找到相应媒体句柄
  作者：       
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_CloseMedia(CMPHandle hPlay)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
    int iRet = 0;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_CloseMedia error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }
	
    ptCmpPlayer->iMonitorPlayThreadRunFlag = 0;
    if (ptCmpPlayer->monitorPlayThreadId)
    {
    	DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "CMP_CloseMedia, monitorPlay thread join begin\n");
        //pthread_join(ptCmpPlayer->monitorPlayThreadId, NULL);
        
        while (0 == ptCmpPlayer->iThreadExitFlag)
        {
        	RTSP_MSleep(15);
        }
        ptCmpPlayer->monitorPlayThreadId = 0;
    	DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "CMP_CloseMedia, monitorPlay thread join end\n");
    }
	
    SetPlayState(ptCmpPlayer, CMP_STATE_STOP);
	iRet = VDEC_StopVideoDec(ptCmpPlayer->VHandle);
    if (iRet < 0)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_CloseMedia error! VDEC_StopVideoDec error, err=%d\n",iRet);
        return CMP_STOPPLAY_ERR;
    }
	
    return 0;
}

/*************************************************
  函数功能:     CMP_GetPlayStatus
  函数描述:     获取播放状态
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:       返回媒体播放状态
  作者：   
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_GetPlayStatus(CMPHandle hPlay)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_GetPlayStatus error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    return ptCmpPlayer->iPlayState;
}

/*************************************************
  函数功能:     CMP_PlayMedia
  函数描述:     开始播放媒体流
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:
  作者：    
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_PlayMedia(CMPHandle hPlay)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
    T_WORK_PACKET tPkt;
	
    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_PlayMedia error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }
    
    tPkt.iMsgCmd = E_PLAY_STATE_PLAY;
    tPkt.dValue = 0;
    PutNodeToWorkQueue(ptCmpPlayer->ptQueue, &tPkt);
    SetPlayState(ptCmpPlayer, CMP_STATE_PLAY);

    return 0;
}

/*************************************************
  函数功能:     CMP_PauseMedia
  函数描述:     暂停播放媒体流
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:
  作者：  
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_PauseMedia(CMPHandle hPlay)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
    T_WORK_PACKET tPkt;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_PauseMedia error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    tPkt.iMsgCmd = E_PLAY_STATE_PAUSE;
    tPkt.dValue = 0;
    PutNodeToWorkQueue(ptCmpPlayer->ptQueue, &tPkt);
    SetPlayState(ptCmpPlayer, CMP_STATE_PAUSE);

    return 0;
}

/*************************************************
  函数功能:     CMP_SetPlaySpeed
  函数描述:     设置播放速度
  输入参数:     hPlay：媒体句柄
  				dSpeed: 速度值
  输出参数:     无
  返回值:
  作者： 
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_SetPlaySpeed(CMPHandle hPlay, double dSpeed)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
    T_WORK_PACKET tPkt;
	
	DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "CMP_SetPlaySpeed, dSpeed=%lf\n", dSpeed);

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_SetPlaySpeed error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    tPkt.iMsgCmd = E_PLAY_STATE_FAST_FORWARD;
    tPkt.dValue = dSpeed;
    PutNodeToWorkQueue(ptCmpPlayer->ptQueue, &tPkt);

	if (dSpeed > 0.124 && dSpeed < 0.126)
	{
		SetPlayState(ptCmpPlayer, CMP_STATE_SLOW_FORWARD);
		ptCmpPlayer->iPlaySpeed = 8;
	}		
	else if (dSpeed > 0.24 && dSpeed < 0.26)
	{
		SetPlayState(ptCmpPlayer, CMP_STATE_SLOW_FORWARD);
		ptCmpPlayer->iPlaySpeed = 4;
	}
	else if (dSpeed > 0.49 && dSpeed < 0.51)
	{
		SetPlayState(ptCmpPlayer, CMP_STATE_SLOW_FORWARD);
		ptCmpPlayer->iPlaySpeed = 2;
	}
	else if (dSpeed > 1.9 && dSpeed < 2.1)
	{
		SetPlayState(ptCmpPlayer, CMP_STATE_FAST_FORWARD);
		ptCmpPlayer->iPlaySpeed = 2;
	}
	else if (dSpeed > 3.9 && dSpeed < 4.1)
	{
		SetPlayState(ptCmpPlayer, CMP_STATE_FAST_FORWARD);
		ptCmpPlayer->iPlaySpeed = 4;
	}

    return 0;
}

/*************************************************
  函数功能:     CMP_SetPosition
  函数描述:     设置播放位置（秒）
  输入参数:     hPlay：媒体句柄
  				nPosTime: 跳转时间(单位S)
  输出参数:     无
  返回值:
  作者：   
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_SetPosition(CMPHandle hPlay, int nPosTime)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;
    T_WORK_PACKET tPkt;
		
    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_SetPosition error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    tPkt.iMsgCmd = E_PLAY_STATE_DRAG_POS;
    tPkt.dValue = (double)nPosTime;
    PutNodeToWorkQueue(ptCmpPlayer->ptQueue, &tPkt);
    SetPlayState(ptCmpPlayer, CMP_STATE_PLAY);
    ptCmpPlayer->iIgnoreFrameNum = 8;
    ptCmpPlayer->iCurrentPlayTime = nPosTime * 1000;

    return 0;
}

/*************************************************
  函数功能:     CMP_GetPlayRange
  函数描述:     获取播放时长
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:       文件播放总时长，以秒为单位
  作者：  
  日期:         2016-11-01
  修改:
*************************************************/
int CMP_GetPlayRange(CMPHandle hPlay)
{
    PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_GetPlayRange error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }

    
    return ptCmpPlayer->iPlayRange;
}

/*************************************************
  函数功能:     CMP_GetCurrentPlayTime
  函数描述:     获取录像当前播放时间
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:       录像当前播放时间，以秒为单位
  作者：   
  日期:         2017-5-12
  修改:
*************************************************/
int CMP_GetCurrentPlayTime(CMPHandle hPlay)
{
	PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_GetCurrentPlayTime error! media is not exist\n");
        return CMP_MEDIAHANDLE_NOTEXIST;
    }
	
	//DebugPrint(DEBUG_CMPLAYER_NORMAL_PRINT, "CMP_GetCurrentPlayTime Ok! iCurrentPlayTime=%d\n", ptCmpPlayer->iCurrentPlayTime);
	return ptCmpPlayer->iCurrentPlayTime / 1000;
}

/*************************************************
  函数功能:     CMP_GetDecState
  函数描述:     获取码流状态(是否有流)
  输入参数:     hPlay：媒体句柄
  输出参数:     无
  返回值:       是否有流的状态，1-有流，0-无流
  作者：   
  日期:         2017-5-12
  修改:
*************************************************/
int CMP_GetStreamState(CMPHandle hPlay)    
{
	PT_CMP_PLAYER_INFO ptCmpPlayer = (PT_CMP_PLAYER_INFO)hPlay;

    if (NULL == ptCmpPlayer)
    {
    	DebugPrint(DEBUG_CMPLAYER_ERROR_PRINT, "CMP_GetDecState error! media is not exist\n");
        return 0;
    }
	
    return ptCmpPlayer->iStreamState;
}

