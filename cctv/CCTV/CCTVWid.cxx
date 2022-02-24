#include "CCTVWid.h"
#include "PasswordWid.h"
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include<sys/types.h>
#include<dirent.h>


#include "state.h"
#include "CMPlayer.h"

#include "pmsgproc.h"
#include "log.h"
#include "NVRMsgProc.h"
#include "debug.h"
#include "lmsgProc.h"
#include "state.h"
#include "msgapp.h"


static   CMPHandle	g_pHplay[8] = {0};	  //最多只能同时存在4个播放句柄，多了会出问题

static int	g_iWarnNum = 0;
static int	g_iWarn = 0;			//是否有报警

static PMSG_HANDLE 	 g_hResUpdate;	   //资源更新的信号句柄

static int	g_iNeedUpdateWarnIcon = 0; //是否需要更新报警图标

static E_PLAY_STYLE g_eCurPlayStyle = E_FOUR_VPLAY;   //当前播放风格
static E_PLAY_STYLE g_eNextPlayStyle = g_eCurPlayStyle;
static int  g_aiCurFourVideoIdx[4] = {-1,-1,-1,-1};
static int  g_aiNextFourVideoIdx[4] = {-1,-1,-1,-1};
static int  g_iCurSingleVideoIdx = -1;
static int  g_iNextSingleVideoIdx = -1;
static int  g_iVideoCycleFlag = 1;
static CMPHandle	g_hSinglePlay = 0;
static char g_acCCTVVersion[28] ={0};
static int	g_iPsdRspType = 0; //0未知  1影音管理  2切换DMI          3切换HMI 
static CMPHandle	g_hBackSinglePlay = 0;
static int	g_iBackSingleVideoIdx = 0;
static int  g_aiBackFourVideoIdx[4] = {-1,-1,-1,-1};
static int  g_iWarnFreshed = 0;  //避免报警信息画面还未刷新，就被别的指令破坏
static int g_iCycTime = 30;
static int g_iMSVideoIndex = -1; //主从交换的相机序号


void CycControlTimer(void *arg);
void UpdateCamState(void *arg);
void PlayCtrlFun(void *arg);
void UpdateWarnInfoTimer(void *arg);
void UpdatePlayStateTimer(void *arg);
void CheckTimeTimer(void*arg);
void RequestIpcStateTimer(void *arg);
void CheckDispStateTimer(void *arg);
void ChangeWid(Fl_Widget* ,void *pData);


static void SVPlayBtnClicked(Fl_Widget*pBtn,void *pData)
{
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	time_t tTime;
    time(&tTime);
	
	//手动点击时如果点击太快  或者存在报警时 此按钮无效
	//pBtn == NULL时为程序自己更新
	if((tTime == pCCTV->m_tLastTime || g_iWarn))  
	{
		return;
	}
	pCCTV->m_tLastTime  = tTime;
	if(E_SINGLE_VPLAY == g_eCurPlayStyle )
	{
	 	return ;
	}
	else if(E_FOUR_VPLAY == g_eCurPlayStyle)
	{
		pCCTV->m_pBtn2[1]->image(pCCTV->m_pImgBtn1[2]);
		pCCTV->m_pBtn2[1]->redraw();
		pCCTV->m_pBtn2[0]->image(pCCTV->m_pImgBtn1[1]);
		pCCTV->m_pBtn2[0]->redraw();
		g_eNextPlayStyle = E_SINGLE_VPLAY;
		if(g_iCurSingleVideoIdx != -1)
		{
			g_iNextSingleVideoIdx = g_iCurSingleVideoIdx;
		}
		else
		{
			for(int i=0;i<4;i++)
			{
				if(g_aiCurFourVideoIdx[i] != -1)
				{
					g_iNextSingleVideoIdx = g_aiCurFourVideoIdx[i];
					break;
				}
                g_aiNextFourVideoIdx[i] = -1;
			}
		}
		if(-1 == g_iNextSingleVideoIdx)
		{
			g_iNextSingleVideoIdx = 0;
		}
		return ;
	}
}

//手动点击时如果点击太快  或者存在报警时 此按钮无效
static void MVPlayBtnClicked(Fl_Widget *pBtn,void *pData)
{
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	time_t tTime;
    time(&tTime);
	if(tTime == pCCTV->m_tLastTime || g_iWarn)
	{
		return;
	}
	pCCTV->m_tLastTime  = tTime;
	if(E_FOUR_VPLAY == g_eCurPlayStyle)
	{
	 	return ;
	}
	else if(E_SINGLE_VPLAY == g_eCurPlayStyle)
	{	
		int iGroup =-1,iPos =0;
		
		pCCTV->m_pBtn2[0]->image(pCCTV->m_pImgBtn1[0]);
		pCCTV->m_pBtn2[0]->redraw();
		pCCTV->m_pBtn2[1]->image(pCCTV->m_pImgBtn1[3]);
		pCCTV->m_pBtn2[1]->redraw();
		g_eNextPlayStyle = E_FOUR_VPLAY;
		if(-1 != g_iCurSingleVideoIdx)
		{
			GetBtnPoseAccordVideoIdx(g_iCurSingleVideoIdx, &iGroup, &iPos);
		}
		
		if(iGroup <0 || (7== iGroup && GetVideoNum()<=28))
		{
			iGroup=0;
		}

		for(int i=0;i<4;i++)
		{
			g_aiNextFourVideoIdx[i] = GetVideoIdxAccordBtnPose(iGroup,i);
			if(-1 == g_aiNextFourVideoIdx[i] && (0 == iGroup && GetVideoNum() <=28))
			{
				g_aiNextFourVideoIdx[i] = GetVideoIdxAccordBtnPose(7,i);
			}
		}
        g_iNextSingleVideoIdx = -1;
		return ;
	}
}

static int GetNextFourVideo(int*piVideo)
{
    int iGroup = -1,iPos = -1;
    int iFirstVideo =-1;
    int iVideoNum = GetVideoNum();
    
    for(int i=0;i<4;i++)
    {
         if(-1 != g_aiCurFourVideoIdx[i])
         {
              iFirstVideo = g_aiCurFourVideoIdx[i];
         }
    }
    GetBtnPoseAccordVideoIdx(iFirstVideo, &iGroup, &iPos);
    iGroup ++;
    
            //当只有28个相机的时候 要将最后一组的相机数和第一组的相机数合起来
            //所以点击第一组和点击最后一组要显示的四个画面一样
            //这里是将选中第八组也等同于选中第一组
     if((7 == iGroup ) && (iVideoNum <=28))
     {
         iGroup = 0; 
     }
     else if( (8 == iGroup) )
     {
        if(iVideoNum <=28)
        {
            iGroup = 1; 
        }
        else
        {
            iGroup = 0; 
        }
     }

     for(int i=0;i<4;i++)
     {
          piVideo[i] = GetVideoIdxAccordBtnPose(iGroup,i);
          if(-1 == piVideo[i] && (0 == iGroup && GetVideoNum() <=28))
          {
               piVideo[i] = GetVideoIdxAccordBtnPose(7,i);
          }
     }
    return 0;
}

static int GetNextSingleVideo(int *piVideo)
{
    int iGroup = -1,iPos =-1;
		int iVideoIndex = -1;
		int iRet = -1;
		
		GetBtnPoseAccordVideoIdx(g_iCurSingleVideoIdx, &iGroup, &iPos);
		iPos ++;
		if(iPos >=4)
		{
			iPos = 0;
			iGroup ++;
			if(8 == iGroup)
			{
				iGroup = 0;
			}
		}
		//按界面的顺序来轮巡，因为下一个按钮有可能是第8组最后一个，
		//而第八组最后一个可能不存在，所以找两次
		for(int iCount =0;iCount<2;iCount++)  
		{
			for(int i=iGroup;i<8;i++)
			{
				for(int j = iPos;j<4;j++)
				{
					iVideoIndex = GetVideoIdxAccordBtnPose(i, j);
					if(iVideoIndex >=0)
					{
						iRet =0;
						iGroup = i;
						iPos = j;
						break;
					}
				}
				if(0 == iRet)
				{
					break;
				}
				iPos =0;
			}
			if(iRet <0)
			{
				iGroup =0;
				iPos =0;
			}
			else
			{
				break;
			}
		}
		*piVideo = GetVideoIdxAccordBtnPose(iGroup, iPos);
        return 0;
}

void GetBackVideoTimer(void *arg)
{
   if(E_FOUR_VPLAY == g_eCurPlayStyle)
    {
	
        g_iNextSingleVideoIdx = -1; //此时不能单画面显示了
        GetNextFourVideo(g_aiNextFourVideoIdx);
	}
	else
	{
        GetNextSingleVideo(&g_iNextSingleVideoIdx);
	}
    //Fl::repeat_timeout(g_iCycTime-4,CycControlTimer,arg);
    Fl::add_timeout(g_iCycTime,GetBackVideoTimer,arg);
}

void CycControlTimer(void *arg)
{
    My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
    
    if(E_FOUR_VPLAY == g_eCurPlayStyle)
    {
  
        for(int i=0;i<4;i++)
        {
            if(g_pHplay[i+4])
            {
                CMP_CloseMedia(g_pHplay[i+4]);
			    CMP_DestroyMedia(g_pHplay[i+4]);
			    g_pHplay[i+4] = NULL;
            }	
        }
        GetNextFourVideo(g_aiBackFourVideoIdx);
        for(int i=0;i<4;i++)
        {
            if(-1 != g_aiBackFourVideoIdx[i])
            {
                char acUrl[256] = {0};
                GetVideoRtspUrl(g_aiBackFourVideoIdx[i],acUrl,sizeof(acUrl));
			    g_pHplay[i+4] = CMP_CreateMedia(pCCTVWid->m_p4BgPlay[i]);
			    CMP_OpenMediaPreview(g_pHplay[i+4], acUrl, CMP_TCP);
            }
        }
    }
    else
    {
        char acUrl[256] = {0};
        int  iNextSingleIdx;
        
        GetNextSingleVideo(&iNextSingleIdx);

        if(iNextSingleIdx != g_iBackSingleVideoIdx)
        {
            if(g_hBackSinglePlay)
            {
                CMP_CloseMedia(g_hBackSinglePlay);
			    CMP_DestroyMedia(g_hBackSinglePlay);
			    g_hBackSinglePlay = NULL;
            }	
            if(-1 != iNextSingleIdx)
            {
                GetVideoMainRtspUrl(iNextSingleIdx,acUrl,sizeof(acUrl));
	            g_hBackSinglePlay = CMP_CreateMedia(pCCTVWid->m_pSinglePlayBg);
		        CMP_OpenMediaPreview(g_hBackSinglePlay, acUrl, CMP_TCP);
            }
            g_iBackSingleVideoIdx = iNextSingleIdx;
        }
    }

	Fl::repeat_timeout(4,GetBackVideoTimer,arg);
}

static void CloseVideoCyc(void *pData)
{
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	g_iVideoCycleFlag =0;
	
	//Fl::remove_timeout(CycControlTimer,pData);
    Fl::remove_timeout(GetBackVideoTimer,pData);

    pCCTV->m_pBtn2[2]->image(pCCTV->m_pImgBtn1[4]);
	pCCTV->m_pBtn2[2]->redraw();
	pCCTV->m_pBtn2[3]->image(pCCTV->m_pImgBtn1[7]);
	pCCTV->m_pBtn2[3]->redraw();

    for(int i=4;i<8;i++)
    {
         if(g_pHplay[i])
         {
            CMP_CloseMedia(g_pHplay[i]);
			CMP_DestroyMedia(g_pHplay[i]);
			g_pHplay[i] = NULL;	
          }
         g_aiBackFourVideoIdx[i-4] = -1;
   }
   if(g_hBackSinglePlay)
    {
		CMP_CloseMedia(g_hBackSinglePlay);
		CMP_DestroyMedia(g_hBackSinglePlay);
		g_hBackSinglePlay = NULL;	
	}
    g_iBackSingleVideoIdx = -1;
}

static void CycleBtnClicked(Fl_Widget*pBtn,void *pData)
{	
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	time_t tTime;
	
    time(&tTime);
	if(tTime == pCCTV->m_tLastTime || g_iWarn)   
	{
		return;
	}
	pCCTV->m_tLastTime  = tTime;
	if( pBtn == pCCTV->m_pBtn2[2] && !g_iVideoCycleFlag)
	{	
		g_iVideoCycleFlag = 1;
        if(E_FOUR_VPLAY == g_eCurPlayStyle)
        {
            g_iNextSingleVideoIdx = -1; //此时不能单画面显示了
        }
		pCCTV->m_pBtn2[2]->image(pCCTV->m_pImgBtn1[5]);
		pCCTV->m_pBtn2[2]->redraw();
		pCCTV->m_pBtn2[3]->image(pCCTV->m_pImgBtn1[6]);
		pCCTV->m_pBtn2[3]->redraw();
		//Fl::add_timeout(g_iCycTime-4,CycControlTimer,pData);
		Fl::add_timeout(g_iCycTime,GetBackVideoTimer,pData);
	}
	else if((pBtn == pCCTV->m_pBtn2[3]) && g_iVideoCycleFlag)
	{
		CloseVideoCyc(pData);
	}
}

static int FindCameBtnInfo(Fl_Widget*pBtn,void *pData,int &iGroup,int &iNo)
{
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	
	for(int i=0;i<8;i++)
	{
		for(int j=0;j<4;j++)
		{
			if(pCCTV->m_pBtn3[i][j] == pBtn)
			{
				iGroup = i;
				iNo = j;
				return 1;
			}
		}
	}
	return 0;
}

static void CameBtnClicked(Fl_Widget*pBtn,void *pData)
{
	int iGroup =0, iNo=0;
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	time_t tTime;
    time(&tTime);
	if(tTime == pCCTV->m_tLastTime ||  g_iWarnFreshed)
    {
		return;
	}
	pCCTV->m_tLastTime  = tTime;
	if(FindCameBtnInfo(pBtn,pData,iGroup,iNo))
	{
		int iVideoIndex = GetVideoIdxAccordBtnPose(iGroup, iNo);
        
		if(E_SINGLE_VPLAY == g_eCurPlayStyle)  //如果此时为单画面模式
		{
			g_iNextSingleVideoIdx = iVideoIndex;
		}
		else
		{
			if(g_iWarn)
			{
				g_iNextSingleVideoIdx = iVideoIndex;
			}
			else
			{
			    //当只有28个相机的时候 要将最后一组的相机数和第一组的相机数合起来
			    //所以点击第一组和点击最后一组要显示的四个画面一样
			    //这里是将选中第八组也等同于选中第一组
				if(7 == iGroup && GetVideoNum() <=28)
				{	
					iGroup = 0;  
				}
				for(int i=0;i<4;i++)
				{
					g_aiNextFourVideoIdx[i] = GetVideoIdxAccordBtnPose(iGroup,i);
					if(-1 == g_aiNextFourVideoIdx[i] && (0 == iGroup && GetVideoNum() <=28))
					{
						g_aiNextFourVideoIdx[i] = GetVideoIdxAccordBtnPose(7,i);
					}
				}
                g_iNextSingleVideoIdx = -1;
			}
		}
        
        if(g_iVideoCycleFlag)
        {
     //       Fl::remove_timeout(CycControlTimer,pData);
            Fl::remove_timeout(GetBackVideoTimer,pData);
            
            for(int i=4;i<8;i++)
            {
                if(g_pHplay[i])
                {
                    CMP_CloseMedia(g_pHplay[i]);
			        CMP_DestroyMedia(g_pHplay[i]);
			        g_pHplay[i] = NULL;	
                }
                g_aiBackFourVideoIdx[i-4] = -1;
            }
            if(g_hBackSinglePlay)
            {
		        CMP_CloseMedia(g_hBackSinglePlay);
		        CMP_DestroyMedia(g_hBackSinglePlay);
		        g_hBackSinglePlay = NULL;	
	        }
            g_iBackSingleVideoIdx = -1;

            //Fl::add_timeout(g_iCycTime-4,CycControlTimer,pData); 
            Fl::add_timeout(g_iCycTime,GetBackVideoTimer,pData); 
        }
        UpdateCamState(pData);
	}
}

//返回值 0-3为四界面序号 4为单界面
static int FindClickedWid(Fl_Widget* pWid,void *pData)
{
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	for(int i=0;i<4;i++)
	{
		if(pCCTV->m_p4BgPlay[i] == pWid)
		{
			return i;
		}
	}
	if(pWid == pCCTV->m_pSinglePlayBg)
	{
		return 4;
	}
	return -1;
}

static void PlayWidClicked(Fl_Widget* pWid,void *pData)
{
	static struct timeval tNow ;
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
	gettimeofday(&tNow, NULL);
	int idiff = (tNow.tv_sec - pCCTV->m_tPrevClickTime.tv_sec)*1000000 
		+tNow.tv_usec - pCCTV->m_tPrevClickTime.tv_usec;

    if(g_iWarnFreshed)
    {
        return;
    }
        
	if((idiff < 600000 && idiff > -600000) && (E_FOUR_VPLAY == g_eCurPlayStyle))
	{		
		int iPlayWidIndex = FindClickedWid(pWid,pData);
		
		if(-1 != g_iCurSingleVideoIdx)  //当前如果为四画面播放
		{
			g_iNextSingleVideoIdx = -1;
		}
		else if(iPlayWidIndex >=0 && iPlayWidIndex <4)
		{
			if(g_pHplay[iPlayWidIndex] && CMP_GetStreamState(g_pHplay[iPlayWidIndex]))
			{
				g_iNextSingleVideoIdx = g_aiCurFourVideoIdx[iPlayWidIndex];
			}
		}
	}
	pCCTV->m_tPrevClickTime = tNow;
}

//监控系统的返回键
void MoniSysBackBtnClicked (Fl_Widget*o,void *pData)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)pData;
	pCCTVWid->show();   
	pCCTVWid->m_pMoniMgeWid->hide();
    
	Fl::add_timeout(1,CheckTimeTimer,pData);
	Fl::add_timeout(0.2,UpdatePlayStateTimer,pData);
	Fl::add_timeout(3,RequestIpcStateTimer,pData);
	Fl::add_timeout(0.4,UpdateWarnInfoTimer,pData);

    if(E_FOUR_VPLAY == g_eCurPlayStyle)
    {
       int iNeedChange = 0;
        
       for(int i =0;i<4;i++)
	   {
	        //此时为4画面显示
	        if(-1 == g_iNextSingleVideoIdx)
            {
               pCCTVWid->m_p4BgPlay[i]->show();
               if(g_pHplay[i])
               {
                    CMP_SetWndDisplayEnable(g_pHplay[i], 1,1);
               } 
            } 
            //另外一个情况为单画面显示，并且当前显示的画面不在四画面中
            //在按下监控按钮时已将其关闭，所以需再次创建
	   }
       if(-1 != g_iNextSingleVideoIdx )
       {
            char acUrl[256] = {0};
           
		     GetVideoMainRtspUrl(g_iNextSingleVideoIdx,acUrl,sizeof(acUrl));
			 g_hSinglePlay = CMP_CreateMedia(pCCTVWid->m_pSinglePlayBg);
			 CMP_OpenMediaPreview(g_hSinglePlay, acUrl, CMP_TCP);
             CMP_SetWndDisplayEnable(g_hSinglePlay,1,1);
       }
	        //此时为4画面显示
       g_iCurSingleVideoIdx = g_iNextSingleVideoIdx;
    }
    else
    {
        if(g_hSinglePlay)
        {
            CMP_SetWndDisplayEnable(g_hSinglePlay, 1,1);
        }
    }
    
	if(g_iVideoCycleFlag)
	{
		//Fl::add_timeout(g_iCycTime-4,CycControlTimer,pData);
		Fl::add_timeout(g_iCycTime,GetBackVideoTimer,pData);
	}
    g_iNeedUpdateWarnIcon = 1;
}


static void MoniBtnClicked (Fl_Widget*,void *pData)
{
	My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;

	if(g_iVideoCycleFlag)
    {   
        Fl::remove_timeout(GetBackVideoTimer,pData);
        //Fl::remove_timeout(CycControlTimer,pData);
	}
	Fl::remove_timeout(CheckTimeTimer,pData);
	Fl::remove_timeout(UpdatePlayStateTimer,pData);
	Fl::remove_timeout(RequestIpcStateTimer,pData);
	Fl::remove_timeout(UpdateWarnInfoTimer,pData);

    
    if(E_FOUR_VPLAY == g_eCurPlayStyle)
    {
        for(int i =0;i<4;i++)
	    {
	        if(g_pHplay[i])
            {
                CMP_SetWndDisplayEnable(g_pHplay[i], 0,0);
            }   
		    if(g_pHplay[i+4])
		    {
			    CMP_CloseMedia(g_pHplay[i+4]);
			    CMP_DestroyMedia(g_pHplay[i+4]);
			    g_pHplay[i+4] = NULL;	
		    }
            g_aiBackFourVideoIdx[i] = -1;
	   }

        if(g_hSinglePlay)
        {
            g_iCurSingleVideoIdx = -1;
            CMP_CloseMedia(g_hSinglePlay);
			CMP_DestroyMedia(g_hSinglePlay);
			g_hSinglePlay = NULL;	
        }
     }
     else
     {
        if(g_hSinglePlay)
        {
            CMP_SetWndDisplayEnable(g_hSinglePlay, 0,0);
        }
        if(g_hBackSinglePlay)
		{
			 CMP_CloseMedia(g_hBackSinglePlay);
			 CMP_DestroyMedia(g_hBackSinglePlay);
			 g_hBackSinglePlay = NULL;	
		 }
         g_iBackSingleVideoIdx = -1;
    }
    pCCTV->m_pMoniMgeWid->m_pSysManaWid->ClearMessageBox();  
	pCCTV->m_pMoniMgeWid->show();
	pCCTV->hide();
}

static int FindFireBtnIdx(Fl_Widget* pWid,void *pData)
{
    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;

    for(int i=0;i<pCCTV->m_iFireCount;i++)
    {
        if(pWid == pCCTV->m_pBoxFire[i])
        {
            return i;
        }
    }
    return -1;
}

static void FireBtnClicked(Fl_Widget* pWid,void *pData)
{
    if(g_iWarnFreshed)
    {
        return ;
    }

    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
    int iIdx = FindFireBtnIdx(pWid,pData);

    if(iIdx >=0 )
    {
        g_iNextSingleVideoIdx = pCCTV->m_aiFireIdx[iIdx];
    }
}

static int FindDoorBtnIdx(Fl_Widget* pWid,void *pData)
{
    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;

    for(int i=0;i<pCCTV->m_iDoorCount;i++)
    {
        if(pWid == pCCTV->m_pBoxDoor[i])
        {
            return i;
        }
    }
    return -1;
}


static int FindDoorClipBtnIdx(Fl_Widget* pWid,void *pData)
{
    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;

    for(int i=0;i<pCCTV->m_iDoorClipCount;i++)
    {
        if(pWid == pCCTV->m_pBoxDoorClip[i])
        {
            return i;
        }
    }
    return -1;
}


static void DoorBtnClicked(Fl_Widget* pWid,void *pData)
{
    if(g_iWarnFreshed)
    {
        return ;
    }

    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
    int iIdx = FindDoorBtnIdx(pWid,pData);

    if(iIdx >=0 )
    {
        g_iNextSingleVideoIdx = pCCTV->m_aiDoorIdx[iIdx];
    }
}

static void DoorClipBtnClicked(Fl_Widget* pWid,void *pData)
{
    if(g_iWarnFreshed)
    {
        return ;
    }

    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
    int iIdx = FindDoorClipBtnIdx(pWid,pData);

    if(iIdx >=0 )
    {
        g_iNextSingleVideoIdx = pCCTV->m_aiDoorClipIdx[iIdx];
    }
}


static int FindPecuBtnIdx(Fl_Widget* pWid,void *pData)
{
    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;

    for(int i=0;i<pCCTV->m_iPecuCount;i++)
    {
        if(pWid == pCCTV->m_pBoxPecu[i])
        {
            return i;
        }
    }
    return -1;
}


static void PecuBtnClicked(Fl_Widget* pWid,void *pData)
{
    if(g_iWarnFreshed)
    {
        return ;
    }

    My_CCTV_Window *pCCTV = (My_CCTV_Window *)pData;
    int iIdx = FindPecuBtnIdx(pWid,pData);

    if(iIdx >=0 )
    {
        g_iNextSingleVideoIdx = pCCTV->m_aiPecuIdx[iIdx];
    }
}


static void PsdConfirmCBFun(Fl_Widget* pWid,void *pData)
{
	PasswordWid *pPassWid= (PasswordWid*)pWid;
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)pData;
	
	if(1 == pPassWid->GetResult())
	{
		if(1 == g_iPsdRspType)
		{
			pCCTVWid->m_pAVWid->show();
			//pCCTVWid->m_pAVWid->cursor(FL_CURSOR_NONE);	
			pCCTVWid->m_pAVWid->UpdateState();
		}
		else  //这是切换到HMI 或者DMI
		{
			pCCTVWid->show();	
			//pCCTVWid->cursor(FL_CURSOR_NONE);	
			ChangeWid(NULL,pData);
		}
		pPassWid->hide();
	}
	else
	{
		pCCTVWid->show();
		//pCCTVWid->cursor(FL_CURSOR_NONE);	
		pPassWid->hide();
	}
}

void VideoBtnClicked(Fl_Widget*,void *pData)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)pData;

    pCCTVWid->m_pPassWid->SetTiTle("即将进入影音管理界面");
	pCCTVWid->m_pPassWid->SetBoxTipVisible(0);
	pCCTVWid->m_pPassWid->show();
    pCCTVWid->hide();
	g_iPsdRspType = 1;
}

void ChangeWid(Fl_Widget* ,void *pData)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)pData;
	int iCmd	=  0;
	int iState = 0;

	if(3 == g_iPsdRspType)
	{
		iState = DISP_STATE_HMI;
		iCmd = MSG_CCTV2DHMI_ASYNC_SWITCH_HMI;	
	}else if(2 == g_iPsdRspType)
	{
		iState = DISP_STATE_DMI;
		iCmd = MSG_CCTV2DHMI_ASYNC_SWITCH_DMI;
	}
	
	LMSG_SendMsgToDHMI(iCmd ,NULL,0);
	
	SetDisplayState(iState);
}

void HMIBtnClicked(Fl_Widget* pWid,void *pData)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)pData;

    pCCTVWid->m_pPassWid->SetTiTle("即将进入HMI界面");
	pCCTVWid->m_pPassWid->SetBoxTipVisible(1);
	pCCTVWid->m_pPassWid->show();	
	//pCCTVWid->m_pPassWid->cursor(FL_CURSOR_NONE);
    pCCTVWid->hide();
	g_iPsdRspType = 3;
}

void DMIBtnClicked(Fl_Widget* pWid,void *pData)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)pData;

    pCCTVWid->m_pPassWid->SetTiTle("即将进入MMI界面");
	pCCTVWid->m_pPassWid->SetBoxTipVisible(1);
	pCCTVWid->m_pPassWid->show();	
	//pCCTVWid->m_pPassWid->cursor(FL_CURSOR_NONE);
    pCCTVWid->hide();
	g_iPsdRspType = 2;
}


//这个函数是为了避免切换播放画面时，有时候会存在播放界面把报警按钮隐藏的情况
void UpdateWarnBtn(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	for(int i=0;i<pCCTVWid->m_iFireCount;i++)
	{
		pCCTVWid->m_pBoxFire[i]->hide();
		pCCTVWid->m_pBoxFire[i]->show();
	}
	for(int i=0;i<pCCTVWid->m_iDoorCount;i++)
	{
		pCCTVWid->m_pBoxDoor[i]->hide();
		pCCTVWid->m_pBoxDoor[i]->show();
	}

    for(int i=0;i<pCCTVWid->m_iDoorClipCount;i++)
	{
		pCCTVWid->m_pBoxDoorClip[i]->hide();
		pCCTVWid->m_pBoxDoorClip[i]->show();
	}
    
	for(int i=0;i<pCCTVWid->m_iPecuCount;i++)
	{
		pCCTVWid->m_pBoxPecu[i]->hide();
		pCCTVWid->m_pBoxPecu[i]->show();
	}
}


void CheckTimeTimer(void*arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	struct tm tLocalTime;
	char acTime[56] = {0};
 	time_t tTime;
    time(&tTime);

	if (localtime_r(&tTime, &tLocalTime) == NULL)
	{
		Fl::repeat_timeout(1,CheckTimeTimer,arg);
		return ;
	}
	if(pCCTVWid->m_u16Year != tLocalTime.tm_year +1900 || 
		pCCTVWid->m_u8Mon != tLocalTime.tm_mon +1||
		pCCTVWid->m_u8Day != tLocalTime.tm_mday)
	{
		memset(acTime,0,sizeof(acTime));
		sprintf(acTime,"%d-%02d-%02d",tLocalTime.tm_year +1900,tLocalTime.tm_mon +1,tLocalTime.tm_mday);
		pCCTVWid->m_pBoxDate[0]->copy_label(acTime);
		pCCTVWid->m_pBoxDate[0]->redraw();
		pCCTVWid->m_u16Year = tLocalTime.tm_year +1900;
		pCCTVWid->m_u8Mon = tLocalTime.tm_mon +1;
		pCCTVWid->m_u8Day = tLocalTime.tm_mday;
		
	}
	if(pCCTVWid->m_iWeek != tLocalTime.tm_wday)
	{
		pCCTVWid->m_iWeek = tLocalTime.tm_wday;
		switch (tLocalTime.tm_wday)
		{
			case 0:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期天");
				break;
			case 1:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期一");
				break;
			case 2:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期二");
				break;
			case 3:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期三");
				break;
			case 4:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期四");
				break;
			case 5:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期五");
				break;
			case 6:
				pCCTVWid->m_pBoxDate[1]->copy_label("星期六");
				break;
		}
		pCCTVWid->m_pBoxDate[1]->redraw();
	}

	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d:%02d:%02d",tLocalTime.tm_hour,tLocalTime.tm_min,tLocalTime.tm_sec);
	pCCTVWid->m_pBoxDate[2]->copy_label(acTime);
	pCCTVWid->m_pBoxDate[2]->redraw();
	
	Fl::repeat_timeout(1,CheckTimeTimer,arg);
}


void PlayStyleChanged(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
    
	if(g_eNextPlayStyle == E_FOUR_VPLAY)
	{
		//将之前的单画面播放句柄关掉      
		if(g_hSinglePlay)
		{
			CMP_CloseMedia(g_hSinglePlay);
			CMP_DestroyMedia(g_hSinglePlay);
			g_hSinglePlay = NULL;	
		}
        g_iCurSingleVideoIdx = -1;
		g_iNextSingleVideoIdx = g_iCurSingleVideoIdx;

        if(g_hBackSinglePlay)
		{
			CMP_CloseMedia(g_hBackSinglePlay);
			CMP_DestroyMedia(g_hBackSinglePlay);
			g_hBackSinglePlay = NULL;	
		}
        g_iBackSingleVideoIdx = -1;
        
		pCCTVWid->m_pSinglePlayBg->hide();

        for(int i=0;i<4;i++)
        {
            if(g_pHplay[i])
			{
				CMP_CloseMedia(g_pHplay[i]);
				CMP_DestroyMedia(g_pHplay[i]);
				g_pHplay[i] = NULL;	
			}
            if(g_pHplay[i+4])
			{
				CMP_CloseMedia(g_pHplay[i+4]);
				CMP_DestroyMedia(g_pHplay[i+4]);
				g_pHplay[i+4] = NULL;	
			}
            g_aiCurFourVideoIdx[i] = -1;
            g_aiBackFourVideoIdx[i] = -1;
        }
			
		for(int i=0;i<4;i++)
		{
			int  iNextVideoIdx = g_aiNextFourVideoIdx[i];
			char acUrl[256] = {0};
			
			pCCTVWid->m_p4BgPlay[i]->show();
			
			if(iNextVideoIdx != -1)
			{
				GetVideoRtspUrl(iNextVideoIdx,acUrl,sizeof(acUrl));
				g_pHplay[i] = CMP_CreateMedia(pCCTVWid->m_p4BgPlay[i]);
				CMP_OpenMediaPreview(g_pHplay[i], acUrl, CMP_TCP);
				CMP_SetWndDisplayEnable(g_pHplay[i],1,1);
			}
			else
			{
				int ix = pCCTVWid->m_p4BgPlay[i]->x();
				int iy = pCCTVWid->m_p4BgPlay[i]->y();
				int iw = pCCTVWid->m_p4BgPlay[i]->w();
				int ih = pCCTVWid->m_p4BgPlay[i]->h();

                //如果此时这个界面不需要显示视频，需要把此处背景刷黑
				CMP_SetBlackBackground(ix ,iy ,iw ,ih);  
			}
			g_aiCurFourVideoIdx[i] = g_aiNextFourVideoIdx[i];
		}
	}
	else
	{ 
        if(g_hBackSinglePlay)
		{
			CMP_CloseMedia(g_hBackSinglePlay);
			CMP_DestroyMedia(g_hBackSinglePlay);
			g_hBackSinglePlay = NULL;	
		}
        g_iBackSingleVideoIdx = -1;
		
		for(int i=0;i<4;i++)
		{			
			pCCTVWid->m_p4BgPlay[i]->hide();
			if(g_pHplay[i]) 
			{		   
                CMP_CloseMedia(g_pHplay[i]);
				CMP_DestroyMedia(g_pHplay[i]);
				g_pHplay[i] = NULL;	
                g_aiCurFourVideoIdx[i] = -1;
			}
			g_aiNextFourVideoIdx[i] = -1;

            if(g_pHplay[i+4])
            {          
               CMP_CloseMedia(g_pHplay[i+4]);
			   CMP_DestroyMedia(g_pHplay[i+4]);
			   g_pHplay[i+4] = NULL;	
               g_aiBackFourVideoIdx[i] = -1;                    
            }
		}

		pCCTVWid->m_pSinglePlayBg->show();
        
        if(g_iCurSingleVideoIdx != g_iNextSingleVideoIdx)
        {
            char acUrl[256] = {0};
            
            if(g_hSinglePlay)
            {
                CMP_CloseMedia(g_hSinglePlay);
				CMP_DestroyMedia(g_hSinglePlay);
				g_hSinglePlay = NULL;
            }
            
		    GetVideoMainRtspUrl(g_iNextSingleVideoIdx,acUrl,sizeof(acUrl));
            g_hSinglePlay = CMP_CreateMedia(pCCTVWid->m_pSinglePlayBg);
		    CMP_OpenMediaPreview(g_hSinglePlay, acUrl, CMP_TCP);
		    CMP_SetWndDisplayEnable(g_hSinglePlay,1,1);        
        }
        else
        {
            CMP_SetWndDisplayEnable(g_hSinglePlay,1,1);
        }      
		g_iCurSingleVideoIdx = g_iNextSingleVideoIdx;
	}
	g_eCurPlayStyle = g_eNextPlayStyle;
}


//四画面模式下
//如果播放的单画面视频存在于四画面视频中，那么将借用四画面视频的句柄
//如果不存在，则另外新建一个视频句柄（只在四画面报警的情况下才存在）
//反正四画面的句柄保持不懂
void FourPlayStyle(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	int iFourVideoChanged = 0;	
	
	//单画面界面的视频发生了变化
	//从单到四 或从单到单 以及从四到单
    if(g_iNextSingleVideoIdx != g_iCurSingleVideoIdx)
	{
	    if(g_hSinglePlay)   //切换时都需要把前面单播放句柄的先关掉
		{
			CMP_CloseMedia(g_hSinglePlay);
			CMP_DestroyMedia(g_hSinglePlay);
			g_hSinglePlay = NULL;	
		}
		
		if(g_iNextSingleVideoIdx != -1)   	//接下来要播放的为单画面视频
		{ 
             char acUrl[256] = {0};
		     GetVideoMainRtspUrl(g_iNextSingleVideoIdx,acUrl,sizeof(acUrl));
		     //strcpy(acUrl,"rtsp://admin:admin123@192.168.60.67:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif");
		     g_hSinglePlay = CMP_CreateMedia(pCCTVWid->m_pSinglePlayBg);
		     CMP_OpenMediaPreview(g_hSinglePlay, acUrl, CMP_TCP);
		     CMP_SetWndDisplayEnable(g_hSinglePlay,0,0);
            pCCTVWid->m_pSinglePlayBg->show();
			for(int i=0;i<4;i++)
			{   
				if(g_pHplay[i])
				{
					   //不需要播放的四画面视频关闭显示了
					CMP_SetWndDisplayEnable(g_pHplay[i],0,0);   
				}
			
				pCCTVWid->m_p4BgPlay[i]->hide();
			}
			pCCTVWid->m_pSinglePlayBg->show();
            
			CMP_SetWndDisplayEnable(g_hSinglePlay,1,1);
		}
		else   //接下来播放的是四视频
		{
			pCCTVWid->m_pSinglePlayBg->hide();

            for(int i=0;i<4;i++)
			{
				pCCTVWid->m_p4BgPlay[i]->show();
 
                if(g_aiCurFourVideoIdx[i] != g_aiNextFourVideoIdx[i])
		        {
		            if(g_pHplay[i])
			        {       
					    CMP_CloseMedia(g_pHplay[i]);
					    CMP_DestroyMedia(g_pHplay[i]);
                        g_pHplay[i] = NULL;
				    }  
                    g_aiCurFourVideoIdx[i] = -1;
                }
            }
			
			for(int i=0;i<4;i++)
			{
				pCCTVWid->m_p4BgPlay[i]->show();
 
                if(g_aiCurFourVideoIdx[i] != g_aiNextFourVideoIdx[i])
		        {
                    if(g_aiNextFourVideoIdx[i]!=-1)
                    {   
                        int iFindIdx = -1;

                        for(int j=0;j<4;j++)
                        {
                             if(g_aiBackFourVideoIdx[j] == g_aiNextFourVideoIdx[i])
                             {
                                 iFindIdx = j;
                                 break;
                              }
                         }
                         if((iFindIdx != -1) &&g_pHplay[iFindIdx +4]) 
                         {
                            g_pHplay[i] = g_pHplay[iFindIdx +4];
                            g_pHplay[iFindIdx +4] = NULL;
                            g_aiBackFourVideoIdx[iFindIdx] = -1;
                         }
                         else
                         {
                              char acUrl[256] = {0};
                    
                              GetVideoRtspUrl(g_aiNextFourVideoIdx[i] ,acUrl,sizeof(acUrl));
				              g_pHplay[i] = CMP_CreateMedia(pCCTVWid->m_p4BgPlay[i]);
				              CMP_OpenMediaPreview(g_pHplay[i], acUrl, CMP_TCP);
                          }                    
                    }
                    g_aiCurFourVideoIdx[i] = g_aiNextFourVideoIdx[i];
                }
			}
            for(int i=0;i<4;i++)
            {
                if(g_pHplay[i])
				{
					CMP_SetWndDisplayEnable(g_pHplay[i],1,1);
				}
				else
				{
					int ix = pCCTVWid->m_p4BgPlay[i]->x();
					int iy = pCCTVWid->m_p4BgPlay[i]->y();
					int iw = pCCTVWid->m_p4BgPlay[i]->w();
					int ih = pCCTVWid->m_p4BgPlay[i]->h();
					
					CMP_SetBlackBackground(ix ,iy ,iw ,ih);
					//不足四个时候，背景要刷黑，因为报警按钮的背景是透明底色
				}
            }
		}
		g_iCurSingleVideoIdx = g_iNextSingleVideoIdx;
         
        //在四画面与单画面切换时，有可能导致报警图标被隐藏
        g_iNeedUpdateWarnIcon =1;  
	}
    //从四到四
    else if(-1 == g_iCurSingleVideoIdx )
    {

        for(int i=0;i<4;i++) 
		{
           if(g_aiCurFourVideoIdx[i] != g_aiNextFourVideoIdx[i])
		   {
		        if(g_pHplay[i])
		        {       
					CMP_CloseMedia(g_pHplay[i]);
					CMP_DestroyMedia(g_pHplay[i]);
                    g_pHplay[i] = NULL;
			    }  
                g_aiCurFourVideoIdx[i] = -1;
           }
        }
    
        for(int i=0;i<4;i++) 
		{
           if(g_aiCurFourVideoIdx[i] != g_aiNextFourVideoIdx[i])
		   {  
                if(g_aiNextFourVideoIdx[i]!=-1)
                {
                    int iFindIdx = -1;

                   for(int j=0;j<4;j++)
                   {
                        if(g_aiBackFourVideoIdx[j] == g_aiNextFourVideoIdx[i])
                        {
                            iFindIdx = j;
                            break;
                        }
                   }
                   if((iFindIdx != -1) &&g_pHplay[iFindIdx +4]) 
                   {
                        g_pHplay[i] = g_pHplay[iFindIdx +4];
                        g_pHplay[iFindIdx +4] = NULL;
                        g_aiBackFourVideoIdx[iFindIdx] = -1;
                        CMP_SetWndDisplayEnable(g_pHplay[i],1,1);
                   }
                   else
                   {
                        char acUrl[256] = {0};
                    
                        GetVideoRtspUrl(g_aiNextFourVideoIdx[i] ,acUrl,sizeof(acUrl));
				        g_pHplay[i] = CMP_CreateMedia(pCCTVWid->m_p4BgPlay[i]);
				        CMP_OpenMediaPreview(g_pHplay[i], acUrl, CMP_TCP);
                        CMP_SetWndDisplayEnable(g_pHplay[i],1,1);
                   }                    
                }
                g_aiCurFourVideoIdx[i] = g_aiNextFourVideoIdx[i];
                g_iNeedUpdateWarnIcon =1;  
            }
		}
        if(g_iNeedUpdateWarnIcon)
        {
           for(int i=0;i<4;i++)
           {
              if(!g_pHplay[i])
			  {
				int ix = pCCTVWid->m_p4BgPlay[i]->x();
				int iy = pCCTVWid->m_p4BgPlay[i]->y();
				int iw = pCCTVWid->m_p4BgPlay[i]->w();
				int ih = pCCTVWid->m_p4BgPlay[i]->h();	
                
				CMP_SetBlackBackground(ix ,iy ,iw ,ih);
			  }
            }
        }  
    }
}

void SinglePlayStyle(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	if(g_iNextSingleVideoIdx != g_iCurSingleVideoIdx)
	{	
		 int iWarnIdx = -1;
         CMPHandle hTmp = NULL;  //保存当前相机的配置
         int iTmpCurIdx = -1;
         
         //判断当前单报警的相机是哪个
         //一直将其保存，目的是为了点击报警图标时能很快切回去
         if(1 == g_iWarnNum)
         {
            if(1 == pCCTVWid->m_iDoorCount)
            {
                iWarnIdx = pCCTVWid->m_aiDoorIdx[0];
            }
            else if(1 == pCCTVWid->m_iDoorClipCount)
            {
                iWarnIdx = pCCTVWid->m_aiDoorClipIdx[0];
            }
            else if(1 == pCCTVWid->m_iPecuCount)
            {
                 iWarnIdx = pCCTVWid->m_aiPecuIdx[0];
            }
            else if(1 == pCCTVWid->m_iFireCount)
            {
                 iWarnIdx = pCCTVWid->m_aiFireIdx[0];
            }
            g_iNeedUpdateWarnIcon = 1;
         }

        
        if(g_hSinglePlay)
		{
		    //如果当前的不是报警索引
		    if(iWarnIdx != g_iCurSingleVideoIdx)
            {
                CMP_CloseMedia(g_hSinglePlay);
			    CMP_DestroyMedia(g_hSinglePlay);
			    g_hSinglePlay = NULL;	
                g_iCurSingleVideoIdx = -1;
            }
            else
            {
                iTmpCurIdx = g_iCurSingleVideoIdx;
                hTmp = g_hSinglePlay;
                g_hSinglePlay = NULL;
                g_iCurSingleVideoIdx = -1;
                CMP_SetWndDisplayEnable(hTmp,0,0);
            }
		}
        
		if(-1 != g_iNextSingleVideoIdx)
		{
            if(g_iNextSingleVideoIdx == g_iBackSingleVideoIdx)
            {
                g_hSinglePlay = g_hBackSinglePlay;
                g_hBackSinglePlay = NULL;
                g_iBackSingleVideoIdx = -1;
            }
            else
            {
                char acUrl[256] = {0};
                
			    GetVideoMainRtspUrl(g_iNextSingleVideoIdx ,acUrl,sizeof(acUrl));
			    //strcpy(acUrl,"rtsp://admin:admin123@192.168.60.67:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif");
                g_hSinglePlay = CMP_CreateMedia(pCCTVWid->m_pSinglePlayBg);
                CMP_OpenMediaPreview(g_hSinglePlay, acUrl, CMP_TCP);
            }
			CMP_SetWndDisplayEnable(g_hSinglePlay,1,1);
        }

        //如果预存的不是当前报警索引号且存在报警
        if((hTmp != g_hBackSinglePlay) && hTmp ) 
        {
            if(g_hBackSinglePlay)
            {
                CMP_CloseMedia(g_hBackSinglePlay);
			    CMP_DestroyMedia(g_hBackSinglePlay);
            }
			g_hBackSinglePlay = hTmp;
            g_iBackSingleVideoIdx = iTmpCurIdx;
        }
		g_iCurSingleVideoIdx = g_iNextSingleVideoIdx;
	}
}

void PlayCtrlFun(void *arg)
{
	if(g_eCurPlayStyle != g_eNextPlayStyle)
	{
		PlayStyleChanged(arg);
		g_eCurPlayStyle = g_eNextPlayStyle;
		g_iNeedUpdateWarnIcon = 1;
	}

	if(E_FOUR_VPLAY == g_eCurPlayStyle)
	{
		FourPlayStyle(arg);
	}
	else
	{
		SinglePlayStyle(arg);
	}
	
	if(g_iNeedUpdateWarnIcon)  //界面切换时,有可能将报警图标给挡住，所以最好是重新显示一次
	{
		g_iNeedUpdateWarnIcon =0;
		UpdateWarnBtn(arg);
	}
    g_iWarnFreshed = 0;
}
		
void RequestIpcStateTimer(void *arg)
{
	for(int i=0;i<3;i++)
	{
		if(E_SERV_STATUS_CONNECT== NVR_GetConnectStatus(i*2))  //冗余操作  发送一个就好
		{
			NVR_SendCmdInfo(i*2, CLI_SERV_MSG_TYPE_GET_HDISK_STATUS, NULL, 0);
			NVR_SendCmdInfo(i*2, CLI_SERV_MSG_TYPE_GET_IPC_STATUS, NULL, 0);
		}
		else if(E_SERV_STATUS_CONNECT== NVR_GetConnectStatus(i*2+1))
		{
			NVR_SendCmdInfo(i*2+1, CLI_SERV_MSG_TYPE_GET_HDISK_STATUS, NULL, 0);
			NVR_SendCmdInfo(i*2+1, CLI_SERV_MSG_TYPE_GET_IPC_STATUS, NULL, 0);
		}
	}	
	Fl::repeat_timeout(3,RequestIpcStateTimer,arg);
}

void UpdateCamState(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	E_PLAY_STYLE eStyle = g_eCurPlayStyle;
	int iNVRState =0;
	int iNvrNo = 0;
	int iVideoNum = GetVideoNum();
	
	for(int i=0;i<iVideoNum;i++)
	{
		int iGroup= -1,iPos =-1;
		int iOnline = GetVideoOnlineState(i);
		int iShelterWarn = GetVideoWarnState(i);
		int iImgIndex = GetVideoImgIdx(i);
		int iNowPlay = 0;
		
		if(iImgIndex <1 || iImgIndex > 32)
		{
			continue;
		}
		GetBtnPoseAccordVideoIdx(i, &iGroup, &iPos);	
		iNvrNo = GetVideoNvrNo(i);
		iNVRState = NVR_GetConnectStatus((iNvrNo/2)*2);
		if(E_SERV_STATUS_CONNECT != iNVRState)
		{
			iNVRState = NVR_GetConnectStatus((iNvrNo/2)*2+1);
		}
		if(E_SERV_STATUS_CONNECT != iNVRState )
		{
			iOnline = 0;
		}
		
		Fl_Image *pNowImg = pCCTVWid->m_pBtn3[iGroup][iPos]->image();
		Fl_Image *pImg = pNowImg;
		if(E_SINGLE_VPLAY == eStyle )
		{
			if(i == g_iNextSingleVideoIdx)
			{
				iNowPlay = 1;
			}
		}
		else if(E_FOUR_VPLAY == eStyle)
		{	
			if((g_iNextSingleVideoIdx >=0) && (i == g_iNextSingleVideoIdx))
			{
				iNowPlay = 1;
			}
			else if(g_iNextSingleVideoIdx <0)
			{			
				for(int iNum=0;iNum<4;iNum++)
				{
					if(i == g_aiNextFourVideoIdx[iNum])
					{
						iNowPlay = 1;
						break;
					}
				}
			}
			
		}

        if(iNowPlay)
		{
			pImg = iOnline ? pCCTVWid->m_pImgBtn2[iImgIndex-1][0]:pCCTVWid->m_pImgBtn2[iImgIndex-1][1];
		}
		else
		{
			pImg = iOnline ? pCCTVWid->m_pImgBtn2[iImgIndex-1][2]:pCCTVWid->m_pImgBtn2[iImgIndex-1][3];
		}
        
		if(iOnline && iShelterWarn)  //遮挡报警
		{
			pImg = pCCTVWid->m_pImgBtn2[iImgIndex-1][4];
		}

		if(pNowImg != pImg)
		{
			pCCTVWid->m_pBtn3[iGroup][iPos]->image(pImg);
			pCCTVWid->m_pBtn3[iGroup][iPos]->hide();
			pCCTVWid->m_pBtn3[iGroup][iPos]->show();
		}
	}
}

void UpdateWarnInfoTimer(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	char acFireWarnInfo[6] = {0};
	char acDoorWarnInfo[6] = {0};
    char acDoorClipWarnInfo[6] = {0};
	u32int	 iPecuWarnInfo = 0;
	int	 iPecuFirstWarnVideoIdx = -1;
	int  iWarnNum = 0;
	int  aiWarnVideoIdx[36] ={0};  //报警相机的序号  因为不同的报警类型 相机序号有重复所以要判断是否已存在啦
	int  iChanged = 0;
	char aiDoorWarnVideoIdx[48];
    char aiDoorClipWarnVideoIdx[48];
	
	GetAllDoorWarnInfo(acDoorWarnInfo, 6);
	GetAllFireWarnInfo(acFireWarnInfo, 6);
    GetAllDoorClipInfo(acDoorClipWarnInfo, 6);
	iPecuWarnInfo = GetPecuWarnInfo();
	iPecuFirstWarnVideoIdx = GetPecuFirstWarnVideoIdx();

    memset(aiDoorWarnVideoIdx,0xff,sizeof(aiDoorWarnVideoIdx));
    memset(aiDoorClipWarnVideoIdx,0xff,sizeof(aiDoorClipWarnVideoIdx));
		
	if(memcmp(acFireWarnInfo,pCCTVWid->m_acFireWarnInfo,sizeof(acFireWarnInfo))
		|| memcmp(acDoorWarnInfo,pCCTVWid->m_acDoorWarnInfo,sizeof(acDoorWarnInfo))
		|| memcmp(acDoorClipWarnInfo,pCCTVWid->m_acDoorClipWarnInfo,6)
		|| iPecuWarnInfo != pCCTVWid->m_iPecuInfo)
	{
		iChanged = 1;
		memcpy(pCCTVWid->m_acFireWarnInfo,acFireWarnInfo,sizeof(acFireWarnInfo));
		memcpy(pCCTVWid->m_acDoorWarnInfo,acDoorWarnInfo,sizeof(acDoorWarnInfo));
        memcpy(pCCTVWid->m_acDoorClipWarnInfo,acDoorClipWarnInfo,sizeof(acDoorClipWarnInfo));
		pCCTVWid->m_iPecuInfo = iPecuWarnInfo;
	}

	if(iChanged)
	{
		int iCount = 0;
        int iDoorClipCount = 0;
		int iFirst = 1;
        g_iWarnFreshed = 1;
        
		if(iPecuFirstWarnVideoIdx >=0)
		{
			aiWarnVideoIdx[iWarnNum] = iPecuFirstWarnVideoIdx;	//最先PECU报警的放最前
			iWarnNum ++;
		}
		
		for(int i=0;i<24;i++)
		{
			pCCTVWid->m_pBoxPecu[i]->hide();
			pCCTVWid->m_pBoxDoor[i]->hide();  //门禁的也在这里影藏掉
			pCCTVWid->m_pBoxDoorClip[i]->hide();
			if(iPecuWarnInfo & (0x1 << i))
			{
				char acBuf[8] = {0};
				int iVideoIdx = GetPecuVideoIndex(i);
				int iHaveExist = 0;  //避免报警相机重复
				
				GetVideoName(iVideoIdx, acBuf, sizeof(acBuf));
				
				for(int j=0;j<iWarnNum;j++)
				{
					if(aiWarnVideoIdx[j] == iVideoIdx)
					{
						iHaveExist =1;
						break;
					}
				}
				if(!iHaveExist)  //之前没有重复的
				{
				    pCCTVWid->m_aiPecuIdx[iCount] = iVideoIdx;
					aiWarnVideoIdx[iWarnNum] = iVideoIdx;
					iWarnNum ++;
					pCCTVWid->m_pBoxPecu[iCount]->copy_label(acBuf);
					pCCTVWid->m_pBoxPecu[iCount]->show();
					iCount ++; //pecu报警数加1
					
				}
				//最前的PECU报警也要加进来
				else if(iVideoIdx == iPecuFirstWarnVideoIdx && iFirst)
				{
				    pCCTVWid->m_aiPecuIdx[iCount] = iVideoIdx;
					pCCTVWid->m_pBoxPecu[iCount]->copy_label(acBuf);
					pCCTVWid->m_pBoxPecu[iCount]->show();
					iCount ++;
					iFirst = 0;  
				}
			}		
		}
		pCCTVWid->m_iPecuCount = iCount;

		iCount = 0;
		for(int i=0;i<6;i++)
		{
			pCCTVWid->m_pBoxFire[i]->hide();
			if(acFireWarnInfo[i])
			{
				char acBuf[8] = {0};
				int iVideoIdx = GetFireWarnVideoIdx(i);
				int iHaveExist = 0;
				
				GetVideoName(iVideoIdx, acBuf, sizeof(acBuf));
				pCCTVWid->m_pBoxFire[iCount]->copy_label(acBuf);
				pCCTVWid->m_pBoxFire[iCount]->show();
                pCCTVWid->m_aiFireIdx[iCount] = iVideoIdx;
				iCount ++;

				for(int i=0;i<iWarnNum;i++)
				{
					if(aiWarnVideoIdx[i] == iVideoIdx)
					{
						iHaveExist =1;
						break;
					}
				}
				if(!iHaveExist)
				{
					aiWarnVideoIdx[iWarnNum] = iVideoIdx;
					iWarnNum ++;
				}
			}
		}
		pCCTVWid->m_iFireCount = iCount;

		iCount = 0;
		for(int i=0;i<6;i++)
			for(int j=0;j<8;j++)
		{
			if(acDoorWarnInfo[i] & (0x01<<j))
			{
				char acBuf[8] = {0};
				int iHaveExist = 0;
				int iVideoIdx = GetDoorWarnVideoIdx(i,j);

				for(int i=0;i<iCount;i++)
				{
					if(aiDoorWarnVideoIdx[i] == iVideoIdx)
					{
						iHaveExist =1;
						break;
					}
				}
				if(!iHaveExist)
				{
					GetVideoName(iVideoIdx, acBuf, sizeof(acBuf));
					pCCTVWid->m_pBoxDoor[iCount]->copy_label(acBuf);
					pCCTVWid->m_pBoxDoor[iCount]->show();
					aiDoorWarnVideoIdx[iCount] = iVideoIdx;
                    pCCTVWid->m_aiDoorIdx[iCount] = iVideoIdx;
					iCount ++;
					
					for(int i=0;i<iWarnNum;i++)
					{
						if(aiWarnVideoIdx[i] == iVideoIdx)
						{
							iHaveExist =1;
							break;
						}
					}
					if(!iHaveExist)
					{
						aiWarnVideoIdx[iWarnNum] = iVideoIdx;
						iWarnNum ++;
					}
				}
			}

            if(acDoorClipWarnInfo[i] & (0x01<<j))
			{
				char acBuf[8] = {0};
				int iHaveExist = 0;
				int iVideoIdx = GetDoorWarnVideoIdx(i,j);

				for(int i=0;i<iDoorClipCount;i++)
				{
					if(aiDoorClipWarnVideoIdx[i] == iVideoIdx)
					{
						iHaveExist =1;
						break;
					}
				}
				if(!iHaveExist)
				{
					GetVideoName(iVideoIdx, acBuf, sizeof(acBuf));
					pCCTVWid->m_pBoxDoorClip[iDoorClipCount]->copy_label(acBuf);
					pCCTVWid->m_pBoxDoorClip[iDoorClipCount]->show();
					aiDoorClipWarnVideoIdx[iDoorClipCount] = iVideoIdx;
                    pCCTVWid->m_aiDoorClipIdx[iDoorClipCount] = iVideoIdx;
					iDoorClipCount ++;
					
					for(int i=0;i<iWarnNum;i++)
					{
						if(aiWarnVideoIdx[i] == iVideoIdx)
						{
							iHaveExist =1;
							break;
						}
					}
					if(!iHaveExist)
					{
						aiWarnVideoIdx[iWarnNum] = iVideoIdx;
						iWarnNum ++;
					}
				}
			}
		}
		pCCTVWid->m_iDoorCount = iCount;       
		pCCTVWid->m_iDoorClipCount = iDoorClipCount;

		if(iWarnNum >1)
		{
		    int iFreshed = 0;
            
			if(g_iVideoCycleFlag)
			{
				CloseVideoCyc(arg);
			}
			if(E_SINGLE_VPLAY == g_eCurPlayStyle)
			{
				pCCTVWid->m_pBtn2[0]->image(pCCTVWid->m_pImgBtn1[0]);
				pCCTVWid->m_pBtn2[0]->redraw();
				pCCTVWid->m_pBtn2[1]->image(pCCTVWid->m_pImgBtn1[3]);
				pCCTVWid->m_pBtn2[1]->redraw();
				g_eNextPlayStyle = E_FOUR_VPLAY;
				g_iNextSingleVideoIdx  = -1;
			}			

            //先找出不需要动的
			for(int i=0;i<4;i++)
			{
			    int iStillWarn = 0;
                int iChanged = 0;
                
			    if(g_aiCurFourVideoIdx[i] != -1)
                {
                    for(int j=0;j<iWarnNum;j++)
				    {
					    if(aiWarnVideoIdx[j] == g_aiCurFourVideoIdx[i])
					    {
						    g_aiNextFourVideoIdx[i] = aiWarnVideoIdx[j];
						    aiWarnVideoIdx[j] = -1;
						    iStillWarn =1;
						    break;
					    }
				    }
                }     
				if(0 == iStillWarn)
				{
					g_aiNextFourVideoIdx[i] = -1;
				}
                    
			}

            //再将剩下的报警相机放到队列中
			for(int i=0;i<4;i++)
			{
				if(-1 == g_aiNextFourVideoIdx[i])
				{
					for(int j=0;j<iWarnNum;j++)
					{
						if(-1 != aiWarnVideoIdx[j])
						{
							g_aiNextFourVideoIdx[i] =  aiWarnVideoIdx[j];
							aiWarnVideoIdx[j] = -1;
							break;
						}
					}
				}
                
                if(g_aiNextFourVideoIdx[i] != g_aiCurFourVideoIdx[i])
                {
                    iFreshed = 1;
                }
			}

            if(-1 != aiWarnVideoIdx[0])
            {
                g_aiNextFourVideoIdx[0] = aiWarnVideoIdx[0];
                iFreshed = 1;
            }
            
            //只要有一个发生了变化 都需要重新调出四界面
            if(iFreshed)
            {
                g_iNextSingleVideoIdx = -1;
            }
		}
		else if(1 == iWarnNum)
		{
			int iVideoIndex = aiWarnVideoIdx[0];
			
			if(g_iVideoCycleFlag)
			{
				CloseVideoCyc(arg);
			}
			if(E_FOUR_VPLAY == g_eCurPlayStyle)
			{
				pCCTVWid->m_pBtn2[0]->image(pCCTVWid->m_pImgBtn1[1]);
				pCCTVWid->m_pBtn2[0]->redraw();
				pCCTVWid->m_pBtn2[1]->image(pCCTVWid->m_pImgBtn1[2]);
				pCCTVWid->m_pBtn2[1]->redraw();
				g_eNextPlayStyle = E_SINGLE_VPLAY;
			}
			g_iNextSingleVideoIdx = iVideoIndex;
		}	
		else
		{
			if(E_SINGLE_VPLAY == g_eCurPlayStyle)
			{
				if(-1 == g_iCurSingleVideoIdx)
				{
					g_iNextSingleVideoIdx = 0;
				}
			}
			else 
			{
				for(int i=0;i<4;i++)
				{
					g_aiNextFourVideoIdx[i] = GetVideoIdxAccordBtnPose(1,i);
				}
                g_iNextSingleVideoIdx = -1;
			}	
		}
		g_iWarnNum = iWarnNum;
		g_iWarn = iWarnNum >0?1:0;
	}
	Fl::repeat_timeout(0.4,UpdateWarnInfoTimer,arg);
}	

void UpdatePlayStateTimer(void *arg)
{
    UpdateCamState(arg);
	PlayCtrlFun(arg);
	Fl::repeat_timeout(0.15,UpdatePlayStateTimer,arg);
}

void LOGFsyncTimer(void *arg)
{
	LOG_FsyncFile();
	Fl::repeat_timeout(10,LOGFsyncTimer,arg);
}


void CheckDispStateTimer(void *arg)
{
	My_CCTV_Window *pCCTVWid = (My_CCTV_Window *)arg;
	static int s_iState = GetDisplayState();
	int iState = GetDisplayState();
		
	if(s_iState != iState)
	{
		T_LOG_INFO tLog;
		char acDispState[4][8] = {"UNKNOW","MMI","HMI","CCTV"};
		
		memset(&tLog,0,sizeof(T_LOG_INFO));
		tLog.iLogType = LOG_TYPE_EVENT;
		snprintf(tLog.acLogDesc,sizeof(tLog.acLogDesc)-1,"disp changed from %s to %s "
							,acDispState[s_iState],acDispState[iState]);
		LOG_WriteLog(&tLog);
       	pCCTVWid->show();
        
		switch(iState)
    	{
       		case DISP_STATE_DMI:  
       		case DISP_STATE_HMI:
       		{
       			pCCTVWid->hide();
				pCCTVWid->HideCCTVResp();
				break;
       		}
	   		case DISP_STATE_CCTV:
       		default:
       		{
       			pCCTVWid->show();
				pCCTVWid->ShowCCTVResp();
				break;
       		}
    	}
		s_iState = iState;
	}
	Fl::repeat_timeout(0.2,CheckDispStateTimer,arg);
}

static int  find_pid_by_name( char* pidName)
{
    DIR *ptDir;
    struct dirent *ptText;
    pid_t iPid;
    int i=0;
    
     ptDir = opendir("/proc");
     if(ptDir == NULL)
     return -1;
        
     while ((ptText = readdir(ptDir)) != NULL)
     {
            FILE *pFile;
            char acFileName[128];
            char acBuffer[128];
            char acName[128];
    
            memset(acFileName,0,sizeof(acFileName));
            memset(acBuffer,0,sizeof(acBuffer)); 
            memset(acName,0,sizeof(acName));
           
            if((strcmp(ptText->d_name,".")==0)||strcmp(ptText->d_name, "..") == 0)
            {
                continue;
            }
           
            if(!isdigit(*ptText->d_name))
            {
                continue;
            }
           
            sprintf(acFileName, "/proc/%s/status", ptText->d_name);
            if (! (pFile= fopen(acFileName, "r")) ) 
            {
                continue;
            }
            
            if (fgets(acBuffer, 128-1, pFile) == NULL)
            {
                fclose(pFile);
                continue;
            }
            
            fclose(pFile);
            
            sscanf(acBuffer, "%*s %s", acName);
            if (strcmp(acName, pidName) == 0) 
            {
                iPid=strtol(ptText->d_name, NULL, 0);
                closedir(ptDir);
                  
                return iPid;
            }
     }
        
     if (ptDir)
     {
         closedir(ptDir);
     }
           
     return -1;
}

int StartNtp()
{   
   system("/home/user/bin/ntpcli.exe &"); 
}


My_CCTV_Window::My_CCTV_Window(int X, int Y, int W, int H, const char *l )
  	:Fl_Double_Window(X,Y,W,H,l) 
{
	m_tLastTime = 0;	
	gettimeofday(&m_tPrevClickTime, NULL);
	m_iPecuInfo = GetPecuWarnInfo();
	GetAllDoorWarnInfo(m_acDoorWarnInfo,sizeof(m_acDoorWarnInfo));
    GetAllDoorClipInfo(m_acDoorClipWarnInfo, sizeof(m_acDoorClipWarnInfo));
	GetAllFireWarnInfo(m_acFireWarnInfo, sizeof(m_acFireWarnInfo));	
	m_iFireCount = 0;
	m_iDoorCount =0;
	m_iPecuCount = 0;
    m_iDoorClipCount = 0;
	SetUi();

    g_iCycTime = GetCycTime();
	Fl::add_timeout(1,CheckTimeTimer,this);     //日期更新
	Fl::add_timeout(0.15,UpdatePlayStateTimer,this); //播放状态更新 如四画面 单画面 播放按钮状态
	Fl::add_timeout(0.4,UpdateWarnInfoTimer,this);   //报警信息更新
	Fl::add_timeout(2,RequestIpcStateTimer,this);	 //请求相机状态
	Fl::add_timeout(10,LOGFsyncTimer,this);			 //日志刷新
	//Fl::add_timeout(g_iCycTime-4,CycControlTimer,this);
	Fl::add_timeout(g_iCycTime,GetBackVideoTimer,this); 
	Fl::add_timeout(0.3,CheckDispStateTimer, this);  //界面的显示状态 显示CCTV还是D、MHI
}


int My_CCTV_Window::HideCCTVResp()
{	
    int iPid = -1;
    
	if(g_iVideoCycleFlag)
	{
	    Fl::remove_timeout(GetBackVideoTimer,this);
		//Fl::remove_timeout(CycControlTimer,this);
	}
	
	Fl::remove_timeout(CheckTimeTimer,this);
	Fl::remove_timeout(UpdatePlayStateTimer,this);
	Fl::remove_timeout(RequestIpcStateTimer,this);
	Fl::remove_timeout(UpdateWarnInfoTimer,this);
	m_pMoniMgeWid->m_pVidManaWid->closePlayVideo();
    
    m_pAVWid->hide();
    m_pMoniMgeWid->hide();
	m_pPassWid->hide();
	for(int i =0;i<8;i++)
	{
		if(g_pHplay[i])
		{
			CMP_CloseMedia(g_pHplay[i]);
			CMP_DestroyMedia(g_pHplay[i]);
			g_pHplay[i] = NULL;	
		}
	}
    if(g_hSinglePlay)
    {
        CMP_CloseMedia(g_hSinglePlay);
		CMP_DestroyMedia(g_hSinglePlay);
		g_hSinglePlay = NULL;	
    }

    if(g_hBackSinglePlay)
    {
        CMP_CloseMedia(g_hBackSinglePlay);
		CMP_DestroyMedia(g_hBackSinglePlay);
		g_hBackSinglePlay = NULL;	
    }

    g_aiBackFourVideoIdx[0] = -1;
    g_aiBackFourVideoIdx[1] = -1;
    g_aiBackFourVideoIdx[2] = -1;
    g_aiBackFourVideoIdx[3] = -1;
    g_iBackSingleVideoIdx = -1;
    g_iCurSingleVideoIdx = -1;
    g_aiCurFourVideoIdx[0] = -1;
	g_aiCurFourVideoIdx[1] = -1;
	g_aiCurFourVideoIdx[2] = -1;
	g_aiCurFourVideoIdx[3] = -1;

    iPid = find_pid_by_name("ntpcli.exe");
    if(iPid >=0)
    {
        char acStr[24] = {0};

        sprintf(acStr,"kill %d",iPid);
        system(acStr);
    }
	return 0;
}

int My_CCTV_Window::ShowCCTVResp()
{
	Fl::add_timeout(1,CheckTimeTimer,this);
	Fl::add_timeout(0.2,UpdatePlayStateTimer,this);
	Fl::add_timeout(3,RequestIpcStateTimer,this);
	Fl::add_timeout(0.4,UpdateWarnInfoTimer,this);
	if(g_iVideoCycleFlag)
	{
		//Fl::add_timeout(g_iCycTime-4,CycControlTimer,this);
		Fl::add_timeout(g_iCycTime,GetBackVideoTimer,this); 
	}
    
    StartNtp();
    g_iNeedUpdateWarnIcon = 1;
	return 0;
}



My_CCTV_Window::~My_CCTV_Window()
{
	
	
}

int My_CCTV_Window::SetUi()
{	
	
	char acRunPath[256] = {0};
	char acImgFullName[256] = {0};
	struct tm tLocalTime;
	char acTime[56] = {0};
	char szData[5][256];
	//单画面 四画面 轮巡开 轮巡关
	char pImageName[8][56] = {"res/btn_02_hig.png","res/btn_02_nor.png","res/btn_01_hig.png","res/btn_01_nor.png",
							"res/btn_03_hig.png","res/btn_03_nor.png","res/btn_04_hig.png","res/btn_04_nor.png"};

	begin();
    align(Fl_Align(FL_ALIGN_TOP|FL_ALIGN_INSIDE));
    clear_border();
	color((Fl_Color)0x406AA700);
	
 	time_t tTime;
    time(&tTime);
	
	if (NULL == localtime_r(&tTime, &tLocalTime))
	{
		 return(-1);
	}
	
	m_u16Year = tLocalTime.tm_year +1900;
	m_u8Mon = tLocalTime.tm_mon +1;
	m_u8Day = tLocalTime.tm_mday;
	m_iWeek = tLocalTime.tm_wday;

	memset(acRunPath,0,sizeof(acRunPath));
	GetCCTVRunDir(acRunPath, sizeof(acRunPath));
	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/br1.png",acRunPath);
	//右边背景
	m_pBoxBk1 = new Fl_Box(833, 0, 192, 624);	
	m_pImgBk1 = new Fl_PNG_Image(acImgFullName);
	m_pBoxBk1->image(m_pImgBk1);
	m_pBoxBk1->align(Fl_Align(FL_ALIGN_TOP|FL_ALIGN_INSIDE));

	for(int i=0;i<3;i++)
	{
		if(0 == i)
		{
			memset(acTime,0,sizeof(acTime));
			sprintf(acTime,"%d-%02d-%02d",tLocalTime.tm_year +1900,tLocalTime.tm_mon +1,tLocalTime.tm_mday);
			m_pBoxDate[i] = new Fl_Box(1024-147, 90, 100, 27,strdup(acTime));
		}
		else if(1 == i)
		{
			switch (tLocalTime.tm_wday)
			{
				case 0:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期天")); //星期天
					break;
				case 1:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期一")); //星期一
					break;
				case 2:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期二")); //星期二
					break;
				case 3:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期三")); //星期三
					break;
				case 4:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期四")); //星期四
					break;
				case 5:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期五")); //星期五
					break;
				case 6:
					m_pBoxDate[i] = new Fl_Box(1024-147, 117, 100, 27,strdup("星期六")); //星期六
					break;
			}
		}
		else if(2 == i)
		{
			memset(acTime,0,sizeof(acTime));
			sprintf(acTime,"%02d:%02d:%02d",tLocalTime.tm_hour,tLocalTime.tm_min,tLocalTime.tm_sec);
			m_pBoxDate[i] = new Fl_Box(1024-147, 144, 100, 27,strdup(acTime));
		}
		m_pBoxDate[i]->labelcolor(FL_WHITE);
		m_pBoxDate[i]->labelsize(18);
	}
	
	m_pBtn1[0] = new Fl_Button(1024-190, 210, 187, 70, strdup("监控管理")); //监控管理
	m_pBtn1[1] = new Fl_Button(1024-190, 290, 187, 70, strdup("影音管理")); //影音管理
	m_pBtn1[2] = new Fl_Button(1024-187, 543, 90, 73, strdup("HMI"));
	m_pBtn1[3] = new Fl_Button(1024-93, 543, 90, 73, strdup("MMI"));
	m_pBtn1[0]->callback(MoniBtnClicked,this);
	m_pBtn1[1]->callback(VideoBtnClicked,this);
	m_pBtn1[2]->callback(HMIBtnClicked,this);
	m_pBtn1[3]->callback(DMIBtnClicked,this);
		
	for(int i=0;i<4;i++)
	{
		m_pBtn1[i]->box(FL_BORDER_BOX);
		m_pBtn1[i]->down_box(FL_DOWN_BOX);
		m_pBtn1[i]->color((Fl_Color)181);
		m_pBtn1[i]->selection_color((Fl_Color)6);
		m_pBtn1[i]->labelsize(20);
		m_pBtn1[i]->labelcolor(FL_WHITE);
		m_pBtn1[i]->visible_focus(0);
	}
	
	m_pBtn2[0] = new Fl_Button(1024-192, 380, 96, 73);
	m_pBtn2[1] = new Fl_Button(1024-96,380, 96, 73);
	m_pBtn2[2] = new Fl_Button(1024-192,460, 96, 73);
	m_pBtn2[3] = new Fl_Button(1024-96,460, 96, 73);	
	
	for(int i=0;i<4;i++)
	{
		memset(acImgFullName,0,sizeof(acImgFullName));
		snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/%s",acRunPath,pImageName[i*2]);
		m_pImgBtn1[i*2] = new Fl_PNG_Image(acImgFullName);
		memset(acImgFullName,0,sizeof(acImgFullName));
		snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/%s",acRunPath,pImageName[i*2+1]);
		m_pImgBtn1[i*2+1] = new Fl_PNG_Image(acImgFullName);
		m_pBtn2[i]->box(FL_NO_BOX);
		m_pBtn2[i]->image(m_pImgBtn1[i*2]);
		m_pBtn2[i]->visible_focus(0);
	}
		
	m_pBtn2[1]->image(m_pImgBtn1[3]);
	m_pBtn2[2]->image(m_pImgBtn1[5]);
	m_pBtn2[0]->callback(SVPlayBtnClicked,this); //单画面被点击
	m_pBtn2[1]->callback(MVPlayBtnClicked,this); //四画面被点击
	m_pBtn2[2]->callback(CycleBtnClicked,this); //轮询开被点击
	m_pBtn2[3]->callback(CycleBtnClicked,this); //轮询关被点击


	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/bg_bottom_2.png",acRunPath);
	m_pImgBk2 = new Fl_PNG_Image(acImgFullName);
		
	m_pBoxBk2 = new Fl_Box(0, 624, 1024, 144);
	m_pBoxBk2->align(Fl_Align(FL_ALIGN_TOP|FL_ALIGN_INSIDE));
	m_pBoxBk2->image(m_pImgBk2);
	
	for(int i=0;i<32;i++)
	{
		memset(szData,0,sizeof(szData));
		snprintf(szData[0],sizeof(szData[0])-1,"%s/res/%d_act.png",acRunPath,i+1);
		snprintf(szData[1],sizeof(szData[0])-1,"%s/res/%d_act_nocon.png",acRunPath,i+1);
		snprintf(szData[2],sizeof(szData[0])-1,"%s/res/%d_noact.png",acRunPath,i+1);
		snprintf(szData[3],sizeof(szData[0])-1,"%s/res/%d_no.png",acRunPath,i+1);
		snprintf(szData[4],sizeof(szData[0])-1,"%s/res/%d_dis.png",acRunPath,i+1);
      	m_pImgBtn2[i][0] = new Fl_PNG_Image(szData[0]);     //在线播放中
		m_pImgBtn2[i][1] = new Fl_PNG_Image(szData[1]);		//不在线播放中
		m_pImgBtn2[i][2] = new Fl_PNG_Image(szData[2]);		//在线未播放
		m_pImgBtn2[i][3] = new Fl_PNG_Image(szData[3]);		//不在线未播放
		m_pImgBtn2[i][4] = new Fl_PNG_Image(szData[4]);		//不在线未播放
	}
	
	for(int i =0;i<8;i++)
	{
		int iLeft = 15;
		
		if(0 == i)
		{
			iLeft = 17;
		}
		m_pBtn3[i][0] = new Fl_Button(iLeft +i*125, 647,60, 50);
		m_pBtn3[i][1] = new Fl_Button(iLeft + i*125, 699,60, 50);
		m_pBtn3[i][2] = new Fl_Button(iLeft +60 +i*125, 647,60, 50);
		m_pBtn3[i][3] = new Fl_Button(iLeft +60+ i*125, 699,60, 50);
		for(int j= 0;j<4;j++)
		{
			int iIndex = GetVideoIdxAccordBtnPose(i, j);
			int iImgIndex = GetVideoImgIdx(iIndex);
			if(iIndex >= 0 && iImgIndex <=32 && iImgIndex >=1)
			{	
				m_pBtn3[i][j]->box(FL_NO_BOX);
				m_pBtn3[i][j]->image(m_pImgBtn2[iImgIndex-1][3]);
				m_pBtn3[i][j]->callback(CameBtnClicked,this);
				m_pBtn3[i][j]->visible_focus(0);	
			}
			else
			{
				m_pBtn3[i][j]->hide();
			}			
		}
	}

	m_p4BgPlay[0] = new My_Play_Box(0,0,416,312);
	m_p4BgPlay[1] = new My_Play_Box(0,312,416,312);
	m_p4BgPlay[2] = new My_Play_Box(416,0,416,312);
	m_p4BgPlay[3] = new My_Play_Box(416,312,416,312);
	m_pSinglePlayBg = new My_Play_Box(0,0,832,624);

	for(int i=0;i<4;i++)
	{
		m_p4BgPlay[i]->box(FL_FLAT_BOX);
		m_p4BgPlay[i]->color(0x00000100);  
		m_p4BgPlay[i]->labelcolor(FL_WHITE);
		m_p4BgPlay[i]->callback(PlayWidClicked,this); //播放的界面被点击  用于全屏切换
	}
	m_pSinglePlayBg->box(FL_FLAT_BOX);
	m_pSinglePlayBg->color(FL_BLACK);
	m_pSinglePlayBg->labelcolor(FL_WHITE);
	m_pSinglePlayBg->callback(PlayWidClicked,this);
	m_pSinglePlayBg->hide();

	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/fire.bmp",acRunPath);
	m_pImgFire = new Fl_BMP_Image(acImgFullName);  
	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/door.bmp",acRunPath);
	m_pImgDoor = new Fl_BMP_Image(acImgFullName);  
    memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/clip.bmp",acRunPath);
	m_pImgDoorClick = new Fl_BMP_Image(acImgFullName); 
	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/pecu.bmp",acRunPath);
	m_pImgPecu = new Fl_BMP_Image(acImgFullName);  
 	
	for(int i=0;i<6;i++)
	{
		m_pBoxFire[i] = new Fl_Button(20, 10 +50*i, 100, 45);
		m_pBoxFire[i]->image(m_pImgFire);
        m_pBoxFire[i]->box(FL_FLAT_BOX);
        m_pBoxFire[i]->down_box(FL_FLAT_BOX);
        m_pBoxFire[i]->color((Fl_Color)0x00000100);
        m_pBoxFire[i]->selection_color((Fl_Color)0x00000100);
      	m_pBoxFire[i]->labelsize(20);
      	m_pBoxFire[i]->labelcolor(FL_BACKGROUND2_COLOR);
      	m_pBoxFire[i]->align(Fl_Align(260|FL_ALIGN_INSIDE));
        m_pBoxFire[i]->visible_focus(0);
        m_pBoxFire[i]->hide();
        m_pBoxFire[i]->callback(FireBtnClicked,this);
	}

	for(int i=0;i<2;i++)
		for(int j=0;j<12;j++)
		{
			m_pBoxDoor[i*12+j] = new Fl_Button(130 + i*115, 10 +50*j, 100, 45);
			m_pBoxDoor[i*12+j]->image(m_pImgDoor);
            m_pBoxDoor[i*12+j]->box(FL_FLAT_BOX);
            m_pBoxDoor[i*12+j]->down_box(FL_FLAT_BOX);
            m_pBoxDoor[i*12+j]->color((Fl_Color)0x00000100);
            m_pBoxDoor[i*12+j]->selection_color((Fl_Color)0x00000100);
			m_pBoxDoor[i*12+j]->labelsize(20);
      		m_pBoxDoor[i*12+j]->labelcolor(FL_BACKGROUND2_COLOR);
      		m_pBoxDoor[i*12+j]->align(Fl_Align(260|FL_ALIGN_INSIDE));
            m_pBoxDoor[i*12+j]->visible_focus(0);
			m_pBoxDoor[i*12+j]->hide();
            m_pBoxDoor[i*12+j]->callback(DoorBtnClicked,this);

            m_pBoxDoorClip[i*12+j] = new Fl_Button(360 + i*115, 10 +50*j, 100, 45);
			m_pBoxDoorClip[i*12+j]->image(m_pImgDoorClick);
            m_pBoxDoorClip[i*12+j]->box(FL_FLAT_BOX);
            m_pBoxDoorClip[i*12+j]->down_box(FL_FLAT_BOX);
            m_pBoxDoorClip[i*12+j]->color((Fl_Color)0x00000100);
            m_pBoxDoorClip[i*12+j]->selection_color((Fl_Color)0x00000100);
			m_pBoxDoorClip[i*12+j]->labelsize(20);
      		m_pBoxDoorClip[i*12+j]->labelcolor(FL_BACKGROUND2_COLOR);
      		m_pBoxDoorClip[i*12+j]->align(Fl_Align(260|FL_ALIGN_INSIDE));
            m_pBoxDoorClip[i*12+j]->visible_focus(0);
			m_pBoxDoorClip[i*12+j]->hide();
            m_pBoxDoorClip[i*12+j]->callback(DoorClipBtnClicked,this);
			
			m_pBoxPecu[i*12+j] = new Fl_Button(590 + i*115, 10+50*j, 100, 45);
			m_pBoxPecu[i*12+j]->image(m_pImgPecu);
            m_pBoxPecu[i*12+j]->box(FL_FLAT_BOX);
            m_pBoxPecu[i*12+j]->down_box(FL_FLAT_BOX);
            m_pBoxPecu[i*12+j]->color((Fl_Color)0x00000100);
            m_pBoxPecu[i*12+j]->selection_color((Fl_Color)0x00000100);
			m_pBoxPecu[i*12+j]->labelsize(20);
      		m_pBoxPecu[i*12+j]->labelcolor(FL_BACKGROUND2_COLOR);
      		m_pBoxPecu[i*12+j]->align(Fl_Align(260|FL_ALIGN_INSIDE));
            m_pBoxPecu[i*12+j]->visible_focus(0);
			m_pBoxPecu[i*12+j]->hide();
            m_pBoxPecu[i*12+j]->callback(PecuBtnClicked,this);
		}
    default_cursor(FL_CURSOR_NONE);
	end();
		
	m_pAVWid = new AVManageWid(0,0,1024,768);
	m_pAVWid->SetDataPtr(this);
	m_pAVWid->hide();
	m_pAVWid->end();
	
	m_pMoniMgeWid = new MoniManageWid(0,0,1024,768);
    m_pMoniMgeWid->SetBackCBFun(MoniSysBackBtnClicked, this);
	m_pMoniMgeWid->hide();
	m_pMoniMgeWid->end();

	m_pPassWid = new PasswordWid(0,0,1024,768);
	m_pPassWid->SetHideCBFun(PsdConfirmCBFun,this);
    m_pPassWid->default_cursor(FL_CURSOR_NONE);
	m_pPassWid->hide();
	m_pPassWid->end();
	
	return 0;
}

static int GetTLCDSoftVersion(char *pcVersion,int iLen)
{
	FILE *fp = 0;
    int iStrLen = 0;
	
	memset(pcVersion,0,iLen);
    fp = fopen("/home/user/version.info", "rb");
    if (NULL == fp)
    {
        printf("[%s]can not open file\n", __FUNCTION__);
        return -1;
    }
    fgets(pcVersion, iLen, fp);
    iStrLen = strlen(pcVersion);
    if (iStrLen > 0)
    {
        if ('\n' == pcVersion[iStrLen - 1])
        {
            pcVersion[iStrLen - 1] = 0;
        }
    }
	return 0;
}


int InitCCTV(void *arg)
{
	T_LOG_INFO tLog;
	T_VIDEO_INFO tVdecInfo;

	DebugInit(13000);
	
	LOG_Init(LOG_FILE_DIR);
    GetTLCDSoftVersion(g_acCCTVVersion,sizeof(g_acCCTVVersion));
	memset(&tLog,0,sizeof(tLog));
	tLog.iLogType = LOG_TYPE_SYS;
	sprintf(tLog.acLogDesc,"cctv %s start",g_acCCTVVersion);
	LOG_WriteLog(&tLog);

	tVdecInfo.iScreenWidth = 1024;
	tVdecInfo.iScreenHeight = 768 ;
	CMP_Init(&tVdecInfo); 
	
	InitPmsgproc();
	
	NVR_init();
	
	g_hResUpdate = PMSG_CreateResConn(12016);

	for(int i=0;i<4;i++)
	{
		g_aiNextFourVideoIdx[i] = GetVideoIdxAccordBtnPose(1,i);
	}

	return 0;
}

