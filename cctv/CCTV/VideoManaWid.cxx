#include "VideoManaWid.h"
#include <string.h>  
#include <stdio.h>  
#include <time.h>
#include "state.h"
#include "NVRMsgProc.h"
#include <arpa/inet.h>
#include "ftpApi.h"
#include "log.h"
#include "comm.h"
#include "debug.h"

#define FTP_SERVER_PORT  21   //FTP服务器默认通信端?

static PFTP_HANDLE g_ftpHandle = 0;
static DateChoiceWid *g_pDateChoiceWid = NULL;


void DateWidCBFun(Fl_Widget* pWid,void *pData)
{
	DateChoiceWid *pDateWid= (DateChoiceWid*)pWid;
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	Fl_Button *pBtn =  pMyVidWid->m_pBtnTime[pMyVidWid->m_iSelIdxTime];
	T_TIME_INFO *ptTime = pMyVidWid->m_iSelIdxTime?&pMyVidWid->m_tEndTime:&pMyVidWid->m_tStartTime;
	if(1 == pDateWid->GetResult())
	{
		char acTime[128] = {0};
		pDateWid->GetTimeStr(acTime);
		ptTime->year = pDateWid->GetYear();
		ptTime->mon = pDateWid->GetMonth();
		ptTime->day = pDateWid->GetDay();
		ptTime->hour = pDateWid->GetHour();
		ptTime->min = pDateWid->GetMin();
		ptTime->sec = pDateWid->GetSec();
		pBtn->copy_label(acTime);
		pBtn->redraw();
	} 
}

static void FileSrhResultTimer(void *arg)
{
	VideoManageWid *pVideoWid= (VideoManageWid*)arg;
	MyTable *pTable = pVideoWid->m_pFilelsTable;
	int iNvrNo = pVideoWid->m_iNvrNo;
	
	pVideoWid->m_iWaitFileCnt ++;
	if(pVideoWid->m_iWaitFileCnt < 50)
	{
		int iDataLen = 0;
		char *pBefore = NULL;
		char *pAfter = NULL;
		int iFirst = 1;
		int iFind = 0;
		T_CMD_PACKET tPkt;
		int iLeaveDataLen = 0;
		char acData[2048] = {0};
		char acData_BK[2048] = {0};
		
		tPkt.iDataLen =0;
		tPkt.pData = NULL;
		
		while(NVR_GetFileInfo(iNvrNo, &tPkt))
		{ 	
			if(iFirst)
			{
				iFind = 1;
				iFirst = 0;
				sleep(1);
			}
		    
			if(tPkt.iDataLen + iLeaveDataLen < 2048)
			{
				memcpy(acData+iLeaveDataLen,tPkt.pData,tPkt.iDataLen);
				iDataLen = tPkt.iDataLen + iLeaveDataLen;
			}
			else
			{
				memcpy(acData,tPkt.pData,tPkt.iDataLen);
				iDataLen = tPkt.iDataLen + iLeaveDataLen;
			}
			iLeaveDataLen = iDataLen;		
			
			pBefore = acData;
			pAfter = pBefore;
			while (*pAfter != 0 && iLeaveDataLen >0)
			{
				string strFile;  
				
				pAfter= strstr(pBefore,".avi");
				if(pAfter)
				{
					strFile.append(pBefore,pAfter -pBefore +4);
					pTable->m_FileString.push_back(strFile);
					iLeaveDataLen -= pAfter-pBefore +4;
					pAfter += 4;
					pBefore = pAfter;
				}								
			}
			if (tPkt.pData)
			{
			    free(tPkt.pData);
				tPkt.pData = NULL;
				tPkt.iDataLen = 0;
			}
			if(iLeaveDataLen >0)
			{
				while((iLeaveDataLen >0) && (0 == acData[iDataLen-iLeaveDataLen]))
				{
					iLeaveDataLen--;
				}
				if(iLeaveDataLen > 0)
				{
					memcpy(acData_BK,&acData[iDataLen-iLeaveDataLen],iLeaveDataLen);
					memset(acData,0,sizeof(acData));
					memcpy(acData,acData_BK,iLeaveDataLen);
				}
				else
				{
					iLeaveDataLen = 0;
				}
			}
			else
			{
				iLeaveDataLen = 0;
			}
		}
		if(iFind)
		{
			pTable->SetSize(pTable->m_FileString.size(),3);
			pTable->col_width(0,40);
			pTable->col_width(1,40);
			if(pTable->m_FileString.size() >13)
            {
                pTable->col_width(2,245);
            }
            else
            {
                pTable->col_width(2,268);
            }
			pTable->redraw();
			SetFileSearchState(E_FILE_IDLE);
			pVideoWid->m_pBoxMessage->copy_label("文件搜索成功");
            pVideoWid->m_pBoxMessage->redraw();
			Fl::remove_timeout(FileSrhResultTimer,arg);
			return;
		}
		Fl::repeat_timeout(0.2,FileSrhResultTimer,arg);
	}
	else
	{
		pVideoWid->m_pBoxMessage->copy_label("文件搜索失败");
        pVideoWid->m_pBoxMessage->redraw();
		SetFileSearchState(E_FILE_IDLE);
		Fl::remove_timeout(FileSrhResultTimer,arg);
	}	
}

static void CheckDownProcessFun(void *arg)
{
	VideoManageWid *pVideoWid= (VideoManageWid*)arg;
	int iProgress = GetFileDownProgress();
	E_FILE_DOWN_STATE eState = GetFileDownState();
	if(iProgress <100)
	{
		pVideoWid->m_pBtnProgress[0]->resize(130,696,iProgress*2,30);
		pVideoWid->m_pBtnProgress[1]->resize(130+iProgress*2,696,200-iProgress*2,30);
		pVideoWid->m_pBtnProgress[0]->redraw();
		pVideoWid->m_pBtnProgress[1]->redraw();
		Fl::repeat_timeout(0.2,CheckDownProcessFun,arg);
	}
	else
	{
		pVideoWid->m_pBtnProgress[0]->resize(130,696,0,30);
		pVideoWid->m_pBtnProgress[1]->resize(130,696,200,30);
		pVideoWid->m_pBtnProgress[0]->hide();
		pVideoWid->m_pBtnProgress[1]->hide();
		pVideoWid->m_pBtnOper[1]->activate();
		
		if(eState == E_FILE_DOWN_SUCC)
		{
			pVideoWid->m_pBoxMessage->copy_label("文件下载成功");
            pVideoWid->m_pBoxMessage->redraw();
		}
		else
		{
			pVideoWid->m_pBoxMessage->copy_label("文件下载失败");
            pVideoWid->m_pBoxMessage->redraw();
		}

		SetFileDownState(E_FILE_DOWN_IDLE);
		FTP_DestoryConnect(g_ftpHandle);
		g_ftpHandle = 0;
		Fl::remove_timeout(CheckDownProcessFun,arg);
        
	}
	return ;
}

static void DateBtnClicked(Fl_Widget* pWid,void *pData)
{	
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;

    if(NULL == g_pDateChoiceWid)
    {
        g_pDateChoiceWid = new DateChoiceWid(370,100,332,430);
	    g_pDateChoiceWid->color((Fl_Color)181);
	    g_pDateChoiceWid->set_modal();
        g_pDateChoiceWid->clear_border();
	    g_pDateChoiceWid->SetDataPtr(pData);
	    g_pDateChoiceWid->SetHideCBFun(DateWidCBFun);	   
        g_pDateChoiceWid->end();
        g_pDateChoiceWid->hide();
    }
    
	if(pWid == pMyVidWid->m_pBtnTime[0])
	{
		pMyVidWid->m_iSelIdxTime = 0;
        g_pDateChoiceWid->position(370, 100);
		g_pDateChoiceWid->SetYear(pMyVidWid->m_tStartTime.year);
		g_pDateChoiceWid->SetMonth(pMyVidWid->m_tStartTime.mon);
		g_pDateChoiceWid->SetDay(pMyVidWid->m_tStartTime.day);
		g_pDateChoiceWid->SetHour(pMyVidWid->m_tStartTime.hour);
		g_pDateChoiceWid->SetMin(pMyVidWid->m_tStartTime.min);
		g_pDateChoiceWid->SetSec(pMyVidWid->m_tStartTime.sec);

	}else if(pWid == pMyVidWid->m_pBtnTime[1])
	{
		pMyVidWid->m_iSelIdxTime = 1;
        g_pDateChoiceWid->position(370, 135);
		g_pDateChoiceWid->SetYear(pMyVidWid->m_tEndTime.year);
		g_pDateChoiceWid->SetMonth(pMyVidWid->m_tEndTime.mon);
		g_pDateChoiceWid->SetDay(pMyVidWid->m_tEndTime.day);
		g_pDateChoiceWid->SetHour(pMyVidWid->m_tEndTime.hour);
		g_pDateChoiceWid->SetMin(pMyVidWid->m_tEndTime.min);
		g_pDateChoiceWid->SetSec(pMyVidWid->m_tEndTime.min);
	}
	g_pDateChoiceWid->show();
}

static void CheckPlayTimer(void *arg)
{
	VideoManageWid *pVideoWid= (VideoManageWid*)arg;	
	if(pVideoWid->m_pHplay)
	{
		pVideoWid->m_iTimeCount ++;
		int iTime = CMP_GetCurrentPlayTime(pVideoWid->m_pHplay);
		if(pVideoWid->m_iTotolTime <= 0)
		{
			//如果打开时没获取到则再取一次
			pVideoWid->m_iTotolTime = CMP_GetPlayRange(pVideoWid->m_pHplay);  
			if(pVideoWid->m_iTotolTime >0 )
			{
				pVideoWid->m_pMyPlaySlider->range(0,pVideoWid->m_iTotolTime);
			}
		}
		pVideoWid->m_pMyPlaySlider->value((pVideoWid->m_iTotolTime<=0)?0:iTime);
        pVideoWid->redraw();
		if(iTime == pVideoWid->m_iTotolTime || ((0 == CMP_GetStreamState(pVideoWid->m_pHplay)) && (pVideoWid->m_iTimeCount >5)))
		{
			CMP_CloseMedia(pVideoWid->m_pHplay);
			CMP_DestroyMedia(pVideoWid->m_pHplay);
			pVideoWid->m_pHplay = 0;
			pVideoWid->m_pBoxSpeed->copy_label("1.00");
			pVideoWid->m_pBoxSpeed->redraw();
			pVideoWid->m_dbSpeed = 1.0;
			pVideoWid->m_pMyPlaySlider->range(0,0);
			pVideoWid->m_pMyPlaySlider->value(0);
			pVideoWid->m_pFilelsTable->SetPlayIndex(-1);
			pVideoWid->m_pFilelsTable->redraw();

            pVideoWid->m_pBoxPlayWid->color(Fl_Color(0x01010100));  
		    pVideoWid->m_pBoxPlayWid->redraw();
            CMP_SetBlackBackground(0,0,1024,768);
			return;
		}
		
		Fl::repeat_timeout(1,CheckPlayTimer,arg);
	}
}

static void CarriageNoChanged(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	int iNvrNo = pMyVidWid->m_pChocie[0]->value();
	pMyVidWid->m_pChocie[1]->clear();
	pMyVidWid->m_pChocie[1]->redraw();	
	int iNum = GetNvrVideoNum(iNvrNo);
	for(int i=0;i<iNum;i++)
	{
		char acName[8]={0};
		int iVideoIdx = GetNvrVideoIdx(iNvrNo, i);
		GetVideoName(iVideoIdx,acName, sizeof(acName)-1);
		pMyVidWid->m_pChocie[1]->add(acName);
	}
}

static void SearchBtnClicked(Fl_Widget* pWid,void *pData)
{
    if(NULL == g_pDateChoiceWid)
    {
        g_pDateChoiceWid = new DateChoiceWid(370,100,332,430);
        g_pDateChoiceWid->color((Fl_Color)181);
        g_pDateChoiceWid->set_modal();
        g_pDateChoiceWid->clear_border();
        g_pDateChoiceWid->SetDataPtr(pData);
        g_pDateChoiceWid->SetHideCBFun(DateWidCBFun);   
            
        g_pDateChoiceWid->end();
        g_pDateChoiceWid->hide();
    }

	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	E_FILE_SEARCH_STATE eState;
	int iNvrNo = pMyVidWid->m_pChocie[0]->value();
	int iIpcPos    = pMyVidWid->m_pChocie[1]->value();
	int iVideoType = pMyVidWid->m_pChocie[2]->value();
	int iCmd = CLI_SERV_MSG_TYPE_GET_RECORD_FILE;
	int iDataLen = sizeof(T_NVR_SEARCH_RECORD);
	int iVideoIdx = -1;
	char acCmdData[96]={0};
	T_NVR_SEARCH_RECORD *pSchFile = NULL;
	int iDiscTime = 0;

	iDiscTime = (pMyVidWid->m_tEndTime.year-pMyVidWid->m_tStartTime.year)*366*24*3600
		+(pMyVidWid->m_tEndTime.mon-pMyVidWid->m_tStartTime.mon)*30*24*3600
		+(pMyVidWid->m_tEndTime.day-pMyVidWid->m_tStartTime.day)*24*3600
		+(pMyVidWid->m_tEndTime.hour-pMyVidWid->m_tStartTime.hour)*3600
		+(pMyVidWid->m_tEndTime.min-pMyVidWid->m_tStartTime.min)*3600
		+(pMyVidWid->m_tEndTime.sec-pMyVidWid->m_tStartTime.sec)*3600;

	if(iDiscTime<=0)
	{
		pMyVidWid->m_pBoxMessage->copy_label("结束时间必须大于开始时间");
        pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}
	
	if(iNvrNo < 0 || iNvrNo > 5)
	{
		pMyVidWid->m_pBoxMessage->copy_label("请选择相应车厢号");
        pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}

	if(iIpcPos < 0)
	{
		pMyVidWid->m_pBoxMessage->copy_label("请选择相应相机");
        pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}
	
	if(iVideoType < 0)
	{
		pMyVidWid->m_pBoxMessage->copy_label("请选择录像类型");
        pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}
	
	eState = GetFileSearchState();
	if(eState != E_FILE_IDLE)
	{
		pMyVidWid->m_pBoxMessage->copy_label("请先等待搜索结果");
        pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}

	if(E_SERV_STATUS_CONNECT != NVR_GetConnectStatus(iNvrNo))  //冗余操作  发送一个就好
	{
		pMyVidWid->m_pBoxMessage->copy_label("此服务器不在线");
		pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}
	
	pMyVidWid->m_iNvrNo = iNvrNo;
	pMyVidWid->m_iWaitFileCnt = 0;
	pMyVidWid->m_pFilelsTable->rows(0); 
	pMyVidWid->m_pFilelsTable->SetPlayIndex(-1);
	pMyVidWid->m_pFilelsTable->redraw();
	pMyVidWid->m_pBoxMessage->copy_label("正在搜索录像文件");
    pMyVidWid->m_pBoxMessage->redraw();
	
	iVideoIdx = GetNvrVideoIdx(pMyVidWid->m_iNvrNo, iIpcPos);

	pSchFile = (T_NVR_SEARCH_RECORD *)acCmdData;
	pSchFile->iCarriageNo = (char)(pMyVidWid->m_iNvrNo +1);
	pSchFile->cVideoType = iVideoType;
	pSchFile->iIpcPos = iVideoIdx+1;
	pSchFile->tStartTime = pMyVidWid->m_tStartTime;
	pSchFile->tEndTime = pMyVidWid->m_tEndTime;
	pSchFile->tStartTime.year = htons(pSchFile->tStartTime.year);
	pSchFile->tEndTime.year = htons(pSchFile->tEndTime.year);
	SetFileSearchState( E_FILE_SEARCHING);
	NVR_CleanFileInfo(iNvrNo);
	
	NVR_SendCmdInfo(iNvrNo, iCmd, acCmdData, iDataLen);
	pMyVidWid->m_pFilelsTable->m_FileString.clear();
    Fl::add_timeout(0.2,FileSrhResultTimer,pData);
    
   /* string strFile1("12.avi"), strFile2("22.avi");
    pMyVidWid->m_pFilelsTable->m_FileString.push_back(strFile1);
    pMyVidWid->m_pFilelsTable->m_FileString.push_back(strFile2);
	pMyVidWid->m_pFilelsTable->SetSize(2,3);
	pMyVidWid->m_pFilelsTable->col_width(0,40);
	pMyVidWid->m_pFilelsTable->col_width(1,40);
	pMyVidWid->m_pFilelsTable->col_width(2,268);
	pMyVidWid->m_pFilelsTable->redraw();
	SetFileSearchState(E_FILE_IDLE);
	pMyVidWid->m_pBoxMessage->copy_label("文件搜索成功");
    pMyVidWid->m_pBoxMessage->redraw();*/

	return;
}

void PftpProc(PFTP_HANDLE PHandle, int iPos)
{
	if(100 == iPos)
	{
		SetFileDownProgress(100);//iPos=100,表示下载完毕。
		SetFileDownState(E_FILE_DOWN_SUCC);
	}
	else if(iPos <0)//暂定iPos=-1表示被告知U盘已拔出, iPos=-2表示被告知U盘写入失败,iPos=-3表示被告知数据接收失败失败。 三种情况都隐藏进度条，并在信号处理函数中销毁FTP连接
	{
		SetFileDownProgress(100);
		SetFileDownState(E_FILE_DOWN_FAIL);
	}
	else
	{
		SetFileDownProgress(iPos);
	}
}

static char *parseFileName(char *pcSrcStr)     //根据录像文件路径全名解析得到单纯的录像文件名
{
    char *pcTmp = NULL;
	if (NULL == pcSrcStr)
	{
		return NULL;
	}
	
    pcTmp = strrchr(pcSrcStr, '/');
    if (NULL == pcTmp)
    {
        return pcSrcStr;
    }

	if (0 == *(pcTmp+1))
	{
		return NULL;
	}
    return pcTmp+1;
}

bool OpenPlayMedia(int iIndex,void *pData)
{
	VideoManageWid *pVideoWid = (VideoManageWid *)pData;
	char szIp[24] = {0};
	int iTime = 0;
    short i16RangeTime = 0;
	char acUrl[256] = {0};
	int iRet = 0;
	char acFileName[256] = {0};
	MyTable *pTable = pVideoWid->m_pFilelsTable;
	pVideoWid->m_iTotolTime =0;
	pVideoWid->m_iTimeCount = 0;
	if(0 != GetNvrIpAddr(pVideoWid->m_iNvrNo,szIp))
	{
		return false;
	}
	strcpy(acFileName,pTable->m_FileString.at(iIndex).c_str());
	Fl::remove_timeout(CheckPlayTimer,pData);
	if (NULL == pVideoWid->m_pHplay)
	{
	    CMP_SetBlackBackground(0,0,1024,768);
		pVideoWid->m_pHplay = CMP_CreateMedia(pVideoWid->m_pBoxPlayWid);
		pVideoWid->m_pMyPlaySlider->step(1);
	}
	else
	{
		CMP_CloseMedia(pVideoWid->m_pHplay);
        CMP_DestroyMedia(pVideoWid->m_pHplay);
        pVideoWid->m_pHplay = CMP_CreateMedia(pVideoWid->m_pBoxPlayWid);
        pVideoWid->m_pMyPlaySlider->step(1);
	}
	sprintf(acUrl,"rtsp://%s:8554/file?name=%s",szIp,acFileName);
	//strcpy(acUrl,"rtsp://192.168.101.81:554//CVMS_RECORD/01+01/20190519_01_01_1105_000123.MP4");
    iRet = CMP_OpenMediaFile(pVideoWid->m_pHplay,acUrl, CMP_TCP);
			
    if(iRet < 0)
    {
        CMP_DestroyMedia(pVideoWid->m_pHplay);

		pVideoWid->m_pHplay = NULL;
		pVideoWid->m_pMyPlaySlider->range(0,0);
		pVideoWid->m_pMyPlaySlider->value(0);
        return false;
    }
	CMP_SetWndDisplayEnable(pVideoWid->m_pHplay,1,1);
    i16RangeTime = CMP_GetPlayRange(pVideoWid->m_pHplay);
    if(i16RangeTime >0)
    {
        iTime = i16RangeTime;
		pVideoWid->m_iTotolTime = i16RangeTime;
    }
    
	pVideoWid->m_pMyPlaySlider->range(0,iTime);
	pVideoWid->m_pMyPlaySlider->value(0);

	pTable->SetPlayIndex(iIndex);
	pTable->redraw();
	
	pVideoWid->m_pBoxSpeed->copy_label("1.00");
	pVideoWid->m_pBoxSpeed->redraw();
	pVideoWid->m_dbSpeed = 1.0;
    pVideoWid->m_pBtnOper[5]->copy_label("暂停");

	Fl::add_timeout(1,CheckPlayTimer,pData);
	return true;
}


void DownBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	int iRet = 0, row = 0;
	char acFullFileName[256] = {0};
	char acSaveFileName[256] = {0};
	char acIpAddr[32] = {0};
	int iSize = pMyVidWid->m_pFilelsTable->rows();	
	MyTable *pTable =  pMyVidWid->m_pFilelsTable;
	int iState = GetFileDownState();

	if ((access(USB_PATH, F_OK) < 0) || (0 == STATE_FindUsbDev()))
	{
		pMyVidWid->m_pBoxMessage->copy_label("请插入U盘!");
        pMyVidWid->m_pBoxMessage->redraw();
		return;
	}

	if(E_FILE_DOWNING == iState)
	{
		pMyVidWid->m_pBoxMessage->copy_label("正在下载,请稍后!");
        pMyVidWid->m_pBoxMessage->redraw();
		return;
	}
	
	if(pMyVidWid->m_iNvrNo <0 || iSize <=0)
	{
	    pMyVidWid->m_pBoxMessage->copy_label("无可下载文件");
        pMyVidWid->m_pBoxMessage->redraw();
		return ;
	}
	
	if (iSize > 0)
	{
		for (row = 0; row <iSize; row++)	 //先判断一次是否没有录像文件被选中，没有则弹框提示
		{
			if (pMyVidWid->m_pFilelsTable->GetFileSelectState(row))
			{
				break;
			}
		}
		if (row == iSize)
		{
			pMyVidWid->m_pBoxMessage->copy_label("请选择相应下载视频");
            pMyVidWid->m_pBoxMessage->redraw();
			return;
		}
		GetNvrIpAddr(pMyVidWid->m_iNvrNo,acIpAddr);
		g_ftpHandle = FTP_CreateConnect(acIpAddr, FTP_SERVER_PORT, PftpProc);
		if (0 == g_ftpHandle)
		{
			pMyVidWid->m_pBoxMessage->copy_label("下载连接失败");
            pMyVidWid->m_pBoxMessage->redraw();
			DebugPrint(DEBUG_FTP_PRINT, "[%s] connect to ftp server:%s error!\n", __FUNCTION__, acIpAddr);
			return;
		}
		for (row = 0; row < iSize; row++)
		{
			if (pTable->GetFileSelectState(row))
			{
				memset(acFullFileName,0,sizeof(acFullFileName));
				strcpy(acFullFileName,pTable->m_FileString.at(row).c_str());
				if (parseFileName(acFullFileName) != NULL)
				{
					snprintf(acSaveFileName, sizeof(acSaveFileName),USB_PATH"%s"
						, parseFileName(acFullFileName));
					DebugPrint(DEBUG_FTP_PRINT, "[%s] add download file:%s!\n", __FUNCTION__, acSaveFileName);
					iRet = FTP_AddDownLoadFile(g_ftpHandle,acFullFileName, acSaveFileName);
					if (iRet < 0)
					{		
						FTP_DestoryConnect(g_ftpHandle);
						pMyVidWid->m_pBoxMessage->copy_label("文件下载失败");
                        pMyVidWid->m_pBoxMessage->redraw();
						return;
					}
				}			
			}
		}
		iRet = FTP_FileDownLoad(g_ftpHandle);
		if (iRet < 0)
		{
			FTP_DestoryConnect(g_ftpHandle);
			pMyVidWid->m_pBoxMessage->copy_label("文件下载失败");
            pMyVidWid->m_pBoxMessage->redraw();
			return;
		}
		SetFileDownState(E_FILE_DOWNING);
		pMyVidWid->m_pBtnOper[1]->deactivate();
		pMyVidWid->m_pBtnProgress[0]->show();
		pMyVidWid->m_pBtnProgress[1]->show();
		Fl::add_timeout(0.2,CheckDownProcessFun,pData);
		pMyVidWid->m_pBoxMessage->copy_label("文件正在下载");
        pMyVidWid->m_pBoxMessage->redraw();
	}
	else
	{
		pMyVidWid->m_pBoxMessage->copy_label("无可下载文件");
        pMyVidWid->m_pBoxMessage->redraw();
	}

    /*if(E_FILE_DOWNING == iState)
    {
            pMyVidWid->m_pBoxMessage->copy_label("正在下载,请稍后!");
            pMyVidWid->m_pBoxMessage->redraw();
            return;
    }

    g_ftpHandle = FTP_CreateConnect("192.168.101.81", FTP_SERVER_PORT, PftpProc);
    if (0 == g_ftpHandle)
	{
		pMyVidWid->m_pBoxMessage->copy_label("下载连接失败");
        pMyVidWid->m_pBoxMessage->redraw();
		DebugPrint(DEBUG_FTP_PRINT, "[%s] connect to ftp server:%s error!\n", __FUNCTION__, acIpAddr);
		return;
	}
	iRet = FTP_AddDownLoadFile(g_ftpHandle,"/CVMS_RECORD/01+01/20190521_01_01_2212_000123.MP4"
	, "/mnt/mmc/20190521_01_01_2212_000123.MP4");

    iRet = FTP_AddDownLoadFile(g_ftpHandle,"/CVMS_RECORD/01+01/014145.avi"
	, "/mnt/mmc/014145.avi");

	if (iRet < 0)
	{		
	    FTP_DestoryConnect(g_ftpHandle);
		pMyVidWid->m_pBoxMessage->copy_label("文件下载失败");
        pMyVidWid->m_pBoxMessage->redraw();
		return;
	}
		iRet = FTP_FileDownLoad(g_ftpHandle);
		if (iRet < 0)
		{
			FTP_DestoryConnect(g_ftpHandle);
			pMyVidWid->m_pBoxMessage->copy_label("文件下载失败");
            pMyVidWid->m_pBoxMessage->redraw();
			return;
		}
		SetFileDownState(E_FILE_DOWNING);
		pMyVidWid->m_pBtnOper[1]->deactivate();
		pMyVidWid->m_pBtnProgress[0]->show();
		pMyVidWid->m_pBtnProgress[1]->show();
		Fl::add_timeout(0.2,CheckDownProcessFun,pData);
		pMyVidWid->m_pBoxMessage->copy_label("文件正在下载");
        pMyVidWid->m_pBoxMessage->redraw();*/
}

void PrevBtnClicked(Fl_Widget* ,void *pData)
{
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	MyTable *pTable = pMyVidWid->m_pFilelsTable;
	int iPlayFile   = pTable->GetPlayIndex();
	int iSize 		= pTable->rows();	
	if(iPlayFile >0 && iSize >0)
	{
		iPlayFile --;
		OpenPlayMedia(iPlayFile,pData);
	}
}


void SlowBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pVideoWid = (VideoManageWid *)pData;
	if(0 != pVideoWid->m_pHplay && (CMP_GetPlayStatus(pVideoWid->m_pHplay) != CMP_STATE_PAUSE))
	{
		double dbSpeed = pVideoWid->m_dbSpeed;
		char acSpeed[12] = {0};
		if(dbSpeed <0.26)
		{
			return;
		}
		dbSpeed = dbSpeed/2;
		sprintf(acSpeed,"%.2f",dbSpeed);
		pVideoWid->m_dbSpeed = dbSpeed;
		CMP_SetPlaySpeed(pVideoWid->m_pHplay,dbSpeed);
		pVideoWid->m_pBoxSpeed->copy_label(acSpeed);
		pVideoWid->m_pBoxSpeed->hide();
        pVideoWid->m_pBoxSpeed->show();
	}
}

void PlayBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pVideoWid = (VideoManageWid *)pData;
	MyTable *pTable = pVideoWid->m_pFilelsTable;
	int row =0;
	int iSize = pTable->rows();	
    
	for (row = 0; row <iSize; row++)	 
	{
		if (1 == pTable->row_selected(row))
		{
			OpenPlayMedia(row,pData);
			break;
		}
	}	
}

void PauesBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pVideoWid = (VideoManageWid *)pData;
	if(0 != pVideoWid->m_pHplay)
	{
	    if(0 == strcmp(pVideoWid->m_pBtnOper[5]->label(),"暂停"))
        {
            Fl::remove_timeout(CheckPlayTimer,pData);
		    CMP_PauseMedia(pVideoWid->m_pHplay);
            pVideoWid->m_pBtnOper[5]->copy_label("恢复");
        } 
        else
        {
            Fl::add_timeout(1,CheckPlayTimer,pData);
		    CMP_PlayMedia(pVideoWid->m_pHplay);
		    pVideoWid->m_pBoxSpeed->copy_label("1.00");
		    pVideoWid->m_pBoxSpeed->redraw();
		    pVideoWid->m_dbSpeed = 1.0;
            pVideoWid->m_pBtnOper[5]->copy_label("暂停");
        }
	}
}

void StopBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pVideoWid = (VideoManageWid *)pData;
	if(0 != pVideoWid->m_pHplay)
	{
		Fl::remove_timeout(CheckPlayTimer,pData);
		CMP_CloseMedia(pVideoWid->m_pHplay);
		CMP_DestroyMedia(pVideoWid->m_pHplay);
		pVideoWid->m_pHplay = 0;
        pVideoWid->m_pBoxPlayWid->color(Fl_Color(0x01010100));  
		pVideoWid->m_pBoxPlayWid->redraw();
        CMP_SetBlackBackground(0,0,1024,768);
		pVideoWid->m_pFilelsTable->SetPlayIndex(-1);
		pVideoWid->m_pBoxSpeed->copy_label("1.00");
		pVideoWid->m_pBoxSpeed->redraw();
		pVideoWid->m_dbSpeed = 1.0;
		
		pVideoWid->m_pMyPlaySlider->range(0,0);
		pVideoWid->m_pMyPlaySlider->value(0);
        pVideoWid->m_pBtnOper[5]->copy_label("暂停");
	}
}

void QuickBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pVideoWid = (VideoManageWid *)pData;
	if(0 != pVideoWid->m_pHplay && (CMP_GetPlayStatus(pVideoWid->m_pHplay) != CMP_STATE_PAUSE))
	{
		double dbSpeed = pVideoWid->m_dbSpeed;
		char acSpeed[12] = {0};
		if(dbSpeed > 2.1)
		{
			return;
		}
		dbSpeed = dbSpeed*2;
		pVideoWid->m_dbSpeed = dbSpeed;
		sprintf(acSpeed,"%.2f",dbSpeed);

		CMP_SetPlaySpeed(pVideoWid->m_pHplay,dbSpeed);
		pVideoWid->m_pBoxSpeed->copy_label(acSpeed);
		pVideoWid->m_pBoxSpeed->hide();
        pVideoWid->m_pBoxSpeed->show();
	}
}

void NextBtnClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	MyTable *pTable = pMyVidWid->m_pFilelsTable;
	int iPlayFile   = pTable->GetPlayIndex();
	int iSize 		= pTable->rows();	
	if (iPlayFile >= 0 && iPlayFile < iSize - 1)
	{
		iPlayFile ++;
		OpenPlayMedia(iPlayFile,pData);
	}
}

void TimeSliderClicked(Fl_Widget* pWid,void *pData)
{
	VideoManageWid *pMyVidWid = (VideoManageWid *)pData;
	
	if(0 != pMyVidWid->m_pHplay)
	{
	    int iValue = pMyVidWid->m_pMyPlaySlider->value();
        int iRange = CMP_GetPlayRange(pMyVidWid->m_pHplay); 
        if(iValue != iRange)
        {
            CMP_SetPosition(pMyVidWid->m_pHplay,iValue);
        }
	}
}

VideoManageWid::VideoManageWid(int x,int y,int w,int h,const char *l)
: Fl_Group(x,y,w,h,l)
{
	m_dbSpeed = 1.0;
	m_iTotolTime = -1;
	m_iTimeCount = 0;
	begin();
	m_iSelIdxTime = 0;
	m_iNvrNo = -1;
	m_pHplay = 0;
	for(int i=0;i<3;i++)
	{
	    if(2 == i)
        {
            m_pBoxTimeTitle[i] = new Fl_Box(10,60 + i*110+5,85,40);
        }   
        else
        {
            m_pBoxTimeTitle[i] = new Fl_Box(10,60 + i*110,85,40);
        }
		
		m_pBoxTimeTitle[i]->labelcolor(FL_WHITE);
		m_pBoxTimeTitle[i]->labelsize(20);
		m_pBoxTimeTitle[i]->labelfont(FL_BOLD);
	}
	m_pBoxTimeTitle[0]->copy_label("回放时间");
	m_pBoxTimeTitle[1]->copy_label("位置选择");
	m_pBoxTimeTitle[2]->copy_label("文件列表");
	
	struct tm *pLocalTime;
	char acTime[56] = {0};
 	time_t tTime;
    time(&tTime);
    pLocalTime = localtime(&tTime);
	m_tEndTime.year  = pLocalTime->tm_year +1900;
	m_tEndTime.mon	= pLocalTime->tm_mon +1;
	m_tEndTime.day	= pLocalTime->tm_mday;
	m_tEndTime.hour	= 23;
	m_tEndTime.min	= 59;
	m_tEndTime.sec	= 59;

	m_tStartTime.year  = m_tEndTime.year;
	m_tStartTime.mon   = m_tEndTime.mon;
	m_tStartTime.day   = m_tEndTime.day;
	m_tStartTime.hour  = 0;
	m_tStartTime.min   = 0;
	m_tStartTime.sec   = 0;
	
	for(int i =0;i<2;i++)
	{
		memset(acTime,0,sizeof(acTime));
		if(0 == i)
		{
			sprintf(acTime,"%d-%02d-%02d 00:00:00",m_tStartTime.year,m_tStartTime.mon,m_tStartTime.day);
			m_pBoxDate[i] = new Fl_Box(10,100,85,30,"起始日期");
			m_pBtnTime[i] = new Fl_Button(95,100,250,30);
			m_pBtnTime[i]->copy_label(acTime);
		}
		else if(1 == i)
		{
			sprintf(acTime,"%d-%02d-%02d 23:59:59",m_tStartTime.year,m_tStartTime.mon,m_tStartTime.day);
			m_pBoxDate[i] = new Fl_Box(10,135,85,30,"结束日期");
			m_pBtnTime[i] = new Fl_Button(95,135,250,30);
			m_pBtnTime[i]->copy_label(acTime);
		}
		m_pBoxDate[i]->labelcolor(FL_WHITE);
		m_pBoxDate[i]->labelsize(18);
		m_pBoxDate[i]->labelfont(FL_BOLD);
		
		m_pBtnTime[i]->box(FL_FLAT_BOX);
      	m_pBtnTime[i]->down_box(FL_FLAT_BOX);
      	m_pBtnTime[i]->color(FL_WHITE);
		m_pBtnTime[i]->selection_color(FL_WHITE);
      	m_pBtnTime[i]->labelsize(18);
      	m_pBtnTime[i]->labelcolor(FL_BLACK);
		m_pBtnTime[i]->callback(DateBtnClicked,this);
	}

	m_pChocie[0] = new  Fl_Choice(100,210,90,30,"车厢号   ");
	m_pChocie[1] = new  Fl_Choice(265,210,90,30,"录像机");
	m_pChocie[2] = new  Fl_Choice(100,245,160,30,"录像类型");
	for(int i=0;i<3;i++)
	{
		m_pChocie[i]->labelcolor(FL_WHITE);
		m_pChocie[i]->labelsize(18);
		m_pChocie[i]->labelfont(FL_BOLD);
	}
	for(int i=1;i<7;i++)
	{
		char acData[56] = {0};
		sprintf(acData,"%d车",i);
		m_pChocie[0]->add(acData);
	}
	m_pChocie[2]->add("所有录像");
	m_pChocie[2]->add("普通录像");
	m_pChocie[2]->add("报警录像");
	m_pChocie[2]->add("紧急录像");
	m_pChocie[2]->value(0);
	
	m_pChocie[0]->callback(CarriageNoChanged,this);

	
	m_pFilelsTable = new MyTable(10, 322, 350, 365);
	m_pFilelsTable->SetTableTyle(MyTable::E_VLIST_TABLE);
    m_pFilelsTable->selection_color(FL_YELLOW);
    m_pFilelsTable->when(FL_WHEN_RELEASE|FL_WHEN_CHANGED);
    //m_pFilelsTable->table_box(FL_NO_BOX);
	m_pFilelsTable->color(FL_WHITE);
	m_pFilelsTable->type((Fl_Table_Row::SELECT_SINGLE));
    // ROWS
    m_pFilelsTable->row_header(0);
    m_pFilelsTable->row_resize(0);
    m_pFilelsTable->row_height_all(22);
    // COLS
    m_pFilelsTable->cols(3);
    m_pFilelsTable->col_header(1);
    m_pFilelsTable->col_resize(0);
	m_pFilelsTable->col_header_height(25);
	m_pFilelsTable->col_header_color(FL_WHITE);
    m_pFilelsTable->col_width(0,40);
	m_pFilelsTable->col_width(1,40);
	m_pFilelsTable->col_width(2,268);

	m_pBoxPlayWid = new Fl_Box(368,65,648,560);
	m_pBoxPlayWid->box(FL_FLAT_BOX);
	m_pBoxPlayWid->color(Fl_Color(0x01010100));

	m_pMyPlaySlider = new MySlider(368,634,648,25,"");
	m_pMyPlaySlider->type(FL_HOR_NICE_SLIDER);
    m_pMyPlaySlider->box(FL_FLAT_BOX);
    m_pMyPlaySlider->color((Fl_Color)0x30529200);
    m_pMyPlaySlider->labelsize(8);
	m_pMyPlaySlider->align(Fl_Align(FL_ALIGN_LEFT));
	m_pMyPlaySlider->labelcolor(FL_WHITE);
	m_pMyPlaySlider->labelfont(FL_BOLD);
	m_pMyPlaySlider->range(0,0);
    m_pMyPlaySlider->step(1);
    m_pMyPlaySlider->value(0);
	m_pMyPlaySlider->textcolor(FL_WHITE);
	m_pMyPlaySlider->textsize(16);
	m_pMyPlaySlider->textfont(FL_BOLD);
	m_pMyPlaySlider->SetSliderType(MySlider::E_PROGRESS);
	m_pMyPlaySlider->selection_color((Fl_Color)181);
	m_pMyPlaySlider->callback(TimeSliderClicked,this);
    m_pMyPlaySlider->when(FL_WHEN_RELEASE_ALWAYS);

	m_pBoxMessage = new Fl_Box(10,730,300,38,""); 
	m_pBoxMessage->labelcolor(FL_RED);
	m_pBoxMessage->labelsize(18);
	//m_pBoxMessage->labelfont(FL_BOLD);
	
	for(int i=0;i<10;i++)
	{
		switch(i)
		{
			case 0:
				m_pBtnOper[i] = new Fl_Button(125, 282, 100, 38,"查询");
				break;
			case 1:
				m_pBtnOper[i] = new Fl_Button(10, 692, 100, 38,"下载");
				break;
            case 9:
                m_pBtnOper[i] = new Fl_Button(904,725,100,40,"返回");
                break;
			default:
				m_pBtnOper[i] = new Fl_Button(196 +i*86, 680, 76, 38);
				break;
		}
		m_pBtnOper[i]->box(FL_UP_BOX);
      	m_pBtnOper[i]->down_box(FL_DOWN_BOX);
      	m_pBtnOper[i]->color((Fl_Color)181);
		m_pBtnOper[i]->selection_color((Fl_Color)6);
      	m_pBtnOper[i]->labelsize(16);
      	//m_pBtnOper[i]->labelcolor(FL_WHITE);
		m_pBtnOper[i]->labelfont(FL_BOLD);
	}
	m_pBtnOper[0]->callback(SearchBtnClicked,this);
	m_pBtnOper[1]->callback(DownBtnClicked,this);
	m_pBtnOper[2]->callback(PrevBtnClicked,this);
	m_pBtnOper[3]->callback(SlowBtnClicked,this);
	m_pBtnOper[4]->callback(PlayBtnClicked,this);
	m_pBtnOper[5]->callback(PauesBtnClicked,this);
	m_pBtnOper[6]->callback(StopBtnClicked,this);
	m_pBtnOper[7]->callback(QuickBtnClicked,this);
	m_pBtnOper[8]->callback(NextBtnClicked,this);
	
	m_pBtnOper[2]->copy_label("上一个");   //上一个
	m_pBtnOper[3]->copy_label("慢进");				 //慢进
	m_pBtnOper[4]->copy_label("播放");				 //播放
	m_pBtnOper[5]->copy_label("暂停");				 //暂停
	m_pBtnOper[6]->copy_label("停止");				 //停止
	m_pBtnOper[7]->copy_label("快进");				 //快进
	m_pBtnOper[8]->copy_label("下一个");	 //下一个
	m_pBtnOper[9]->copy_label("返回");	 //下一个

	m_pBoxSpeed = new Fl_Box(968,680,50,30,"1.00");
	m_pBoxSpeed->labelcolor(FL_WHITE);
	m_pBoxSpeed->labelsize(18);
	m_pBoxSpeed->labelfont(FL_BOLD);

	m_pBtnProgress[0] = new Fl_Box(130, 696, 0, 30);
	m_pBtnProgress[1] = new Fl_Box(130, 696, 200, 30);
	m_pBtnProgress[0]->box(FL_FLAT_BOX);
	m_pBtnProgress[1]->box(FL_FLAT_BOX);
	m_pBtnProgress[0]->color(FL_DARK_GREEN);
	m_pBtnProgress[1]->color(FL_WHITE);
	m_pBtnProgress[0]->hide();
	m_pBtnProgress[1]->hide();	

    end();
}

VideoManageWid::~VideoManageWid()
{
	Fl::remove_timeout(CheckDownProcessFun,this);
	Fl::remove_timeout(CheckPlayTimer,this);
	Fl::remove_timeout(FileSrhResultTimer,this);
}

static void StopPlayTimer(void *arg)
{
    VideoManageWid *pVideoWid= (VideoManageWid*)arg;	
    
	StopBtnClicked(pVideoWid->m_pBtnOper[6],arg) ;
    Fl::remove_timeout(StopPlayTimer,arg);
}

int VideoManageWid::closePlayVideo()
{
    if(g_pDateChoiceWid)
    {
       g_pDateChoiceWid->hide();
    }
    if(0 != m_pHplay)
    {
        StopBtnClicked(m_pBtnOper[6],this) ;
        return 1;
    }
    return 0;
}

int VideoManageWid::BeginTimer()
{
	return 0;
}

int VideoManageWid::EndTimer()
{
	return 0;
}




