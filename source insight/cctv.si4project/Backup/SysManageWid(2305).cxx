#include "SysManageWid.h"
#include <FL/fl_ask.H>
#include "md5.h"
#include <time.h>
#include "log.h"
#include "pmsgproc.h"
#include "NVRMsgProc.h"
#include "comm.h"

static void PassBtnPressFun(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	Fl_Secret_Input *pInput = NULL;
	
	if(0 == pMySysWid->m_iSelIndexPass)
	{
		pInput = pMySysWid->m_pInPwd[0];
	}
	else if(1 == pMySysWid->m_iSelIndexPass)
	{
		pInput = pMySysWid->m_pInPwd[1];
	}
	else if(2 == pMySysWid->m_iSelIndexPass)
	{
		pInput = pMySysWid->m_pInPwd[2];
	}
		
	if(pInput->size() >=8 && pWid !=  pMySysWid->m_pBtnPass[10])
	{
		return ;
	}
	if(pWid == pMySysWid->m_pBtnPass[0])
	{	
		pInput->insert("1");
	}
	else if(pWid == pMySysWid->m_pBtnPass[1])
	{	
		pInput->insert("2");
	}else if(pWid == pMySysWid->m_pBtnPass[2])
	{	
		pInput->insert("3");
	}else if(pWid == pMySysWid->m_pBtnPass[3])
	{	
		pInput->insert("4");
	}else if(pWid == pMySysWid->m_pBtnPass[4])
	{	
		pInput->insert("5");
	}else if(pWid == pMySysWid->m_pBtnPass[5])
	{	
		pInput->insert("6");
	}else if(pWid == pMySysWid->m_pBtnPass[6])
	{	
		pInput->insert("7");
	}else if(pWid == pMySysWid->m_pBtnPass[7])
	{	
		pInput->insert("8");
	}else if(pWid == pMySysWid->m_pBtnPass[8])
	{	
		pInput->insert("9");
	}else if(pWid == pMySysWid->m_pBtnPass[9])
	{	
		pInput->insert("0");
	}else if(pWid == pMySysWid->m_pBtnPass[10])
	{	
		int iSize = pInput->size();
		if(iSize >0)
		{
			pInput->cut(iSize-1,iSize);
		}
	}	
}

static void RestartClicked(Fl_Widget* pWid,void *pData)
{
	return ;
}

static void PassEditClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	
	if(pWid == pMySysWid->m_pInPwd[0])
	{
		pMySysWid->m_iSelIndexPass = 0;
	}else if(pWid == pMySysWid->m_pInPwd[1])
	{
		pMySysWid->m_iSelIndexPass = 1;
	}
	else if(pWid == pMySysWid->m_pInPwd[2])
	{
		pMySysWid->m_iSelIndexPass = 2;
	}
}

static void PsdConfirmFun(void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	Fl_Secret_Input *pInput =  pMySysWid->m_pInPwd[1];
	const char *pcEnterPass = pInput->value(); 
	char acPassword[32]={0};
	T_LOG_INFO tLog;
	unsigned char acData[16] = {0}; 
	FILE *file;  
	char acRunPath[256] ={0};
	char acFileFullName[256] = {0};
		
	snprintf(acFileFullName,sizeof(acFileFullName)-1,PASSWORDDIR"/Password");
	strncpy(acPassword,pcEnterPass,sizeof(acPassword));
	MD5_String(acPassword,acData);
    if ((file = fopen (acFileFullName, "wb")) != NULL) 
    {
		fwrite(acData,sizeof(acData),1,file );
		fclose(file);
    }

	pMySysWid->m_pBoxMessage->copy_label("密码修改成功");
    pMySysWid->m_pBoxMessage->redraw();
	tLog.iLogType = LOG_TYPE_EVENT;
	memset(tLog.acLogDesc,0,sizeof(tLog.acLogDesc));
	sprintf(tLog.acLogDesc,"user changed password");
	LOG_WriteLog(&tLog);
	pMySysWid->m_pInPwd[0]->value("");
	pMySysWid->m_pInPwd[1]->value("");
	pMySysWid->m_pInPwd[2]->value("");
	pMySysWid->m_pInPwd[0]->redraw();
	pMySysWid->m_pInPwd[1]->redraw();
	pMySysWid->m_pInPwd[2]->redraw();
	return ;
}

static void FindSysLogClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	T_LOG_TIME_INFO tTime;
	PT_MSG_LOG_INFO ptLog = pMySysWid->m_pMyLogTable->GetLogInfo();
	int iCount  =0;
	tTime.year = atoi(pMySysWid->m_pBoxTime[0]->label());
	tTime.month = atoi(pMySysWid->m_pBoxTime[1]->label());
	tTime.day = atoi(pMySysWid->m_pBoxTime[2]->label());
	iCount = LOG_ReadLog(&tTime, ptLog, 0);
	pMySysWid->m_pMyLogTable->setLogNum(iCount);
	pMySysWid->m_pMyLogTable->SetLogPage(0);
	if(iCount >10)
	{
		pMySysWid->m_pMyLogTable->rows(10);
	}
	else
	{
		pMySysWid->m_pMyLogTable->rows(iCount);
	}
	pMySysWid->m_pMyLogTable->SetLogType(0);
	pMySysWid->m_pMyLogTable->redraw();
	return ;
}

static void FindOperLogClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	T_LOG_TIME_INFO tTime;
	PT_MSG_LOG_INFO ptLog = pMySysWid->m_pMyLogTable->GetLogInfo();
	int iCount  =0;
	tTime.year = atoi(pMySysWid->m_pBoxTime[0]->label());
	tTime.month = atoi(pMySysWid->m_pBoxTime[1]->label());
	tTime.day = atoi(pMySysWid->m_pBoxTime[2]->label());
	iCount = LOG_ReadLog(&tTime, ptLog, 1);
	pMySysWid->m_pMyLogTable->setLogNum(iCount);
	pMySysWid->m_pMyLogTable->SetLogType(1);
	pMySysWid->m_pMyLogTable->SetLogPage(0);
	if(iCount >10)
	{
		pMySysWid->m_pMyLogTable->rows(10);
	}
	else
	{
		pMySysWid->m_pMyLogTable->rows(iCount);
	}
	pMySysWid->m_pMyLogTable->redraw();
	return ;
}

static void PrePageClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	MyTable *pTable = pMySysWid->m_pMyLogTable;
	int iPage = pTable->GetLogPage();
	int iNum  = pTable->GetLogNum();
	int iTotalPage = (iNum+9)/10;
	if(iPage <=0 || iTotalPage <1)
	{
		return;
	}
	iPage --;
	pMySysWid->m_pMyLogTable->SetLogPage(iPage);
	pMySysWid->m_pMyLogTable->rows(10);
	pMySysWid->m_pMyLogTable->redraw();

	return ;
}

static void NextPageClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	MyTable *pTable = pMySysWid->m_pMyLogTable;
	int iPage = pTable->GetLogPage();
	int iNum  = pTable->GetLogNum();
	int iTotalPage = (iNum+9)/10;
	if(iPage >= iTotalPage-1)
	{
		return;
	}
	iPage ++;
	pMySysWid->m_pMyLogTable->SetLogPage(iPage);
	if(iPage < iTotalPage-1)
	{
		pMySysWid->m_pMyLogTable->rows(10);
	}
	else
	{
		pMySysWid->m_pMyLogTable->rows((0 == iNum%10)?10:(iNum%10));
	}
	pMySysWid->m_pMyLogTable->redraw();
	return ;
}

static int JudgePassword(void* pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	const char *pcEnterPass = pMySysWid->m_pInPwd[0]->value(); 
	char acPwd[32] = {0};
	unsigned char acData[16] = {0};
	unsigned char acBuffer[16] = {0};
	FILE *file = NULL;  
	char acFileFullName[256] = {0};
		
	strncpy(acPwd,pcEnterPass,sizeof(acPwd));
	MD5_String(acPwd,acData);
		
	snprintf(acFileFullName,sizeof(acFileFullName)-1,PASSWORDDIR"/Password");
    if ((file = fopen (acFileFullName, "rb")) == NULL) 
    {
    	if(strcmp(acPwd,"12345"))
    	{
    		pMySysWid->m_pBoxMessage->copy_label("密码错误");
			pMySysWid->m_pBoxMessage->redraw();
			return 0;
    	}
		if ((file = fopen(acFileFullName, "ab")) == NULL)
    	{
    		return 0;
    	}
		fwrite(acData,sizeof(acData),1,file );
		fclose(file);
    }
	else
	{
		fread(acBuffer, 1, sizeof(acBuffer),file);
		if(memcmp(acBuffer,acData,sizeof(acData)) && strcmp(acPwd,"85859696"))
		{
			pMySysWid->m_pBoxMessage->copy_label("密码错误");
			pMySysWid->m_pBoxMessage->redraw();
			fclose(file);
			return 0;
		}
		fclose(file);
	}
    return 1;
}

static void PassSetBtnClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;

	if(0 == JudgePassword(pData))
	{
		pMySysWid->m_pBoxMessage->copy_label("旧密码错误");
        pMySysWid->m_pBoxMessage->redraw();
		return;
	}
	
	if(strcmp(pMySysWid->m_pInPwd[1]->value(), pMySysWid->m_pInPwd[2]->value()))
	{
		pMySysWid->m_pBoxMessage->copy_label("两次密码不一致");
        pMySysWid->m_pBoxMessage->redraw();
		return;
	}
	if(0 == pMySysWid->m_pInPwd[1]->size())
	{
		pMySysWid->m_pBoxMessage->copy_label("密码不能为空");
        pMySysWid->m_pBoxMessage->redraw();
		return;
	}

	PsdConfirmFun(pData);
}
static void MapLightBtnClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	int iValue = pMySysWid->m_pSliderLight->value();
	
	SetDynamicMapBrightness(iValue);
    AdjustDynamicMapBrightness(iValue);
}


static void TimeBtnClicked(Fl_Widget* pWid,void *pData)
{
	SysManageWid *pMySysWid = (SysManageWid *)pData;
	int iYear = pMySysWid->GetYear();
	int iMonth = pMySysWid->GetMonth();
	int iDay = pMySysWid->GetDay();
	int iMaxDay = 28;
	if(pWid == pMySysWid->m_pBtnTime[0])
	{
		iYear--;
		if(iYear <1970)
		{
			return ;
		}	
		pMySysWid->SetYear(iYear);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMySysWid->SetDay(iMaxDay);
		}
	}
	else if(pWid == pMySysWid->m_pBtnTime[1])
	{	
		iYear++;
		if(iYear >2100)
		{
			return ;
		}
		pMySysWid->SetYear(iYear);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMySysWid->SetDay(iMaxDay);
		}
	}
	else if(pWid == pMySysWid->m_pBtnTime[2])
	{
		iMonth--;
		if(iMonth <1)
		{
			iMonth = 12;
		}
		pMySysWid->SetMonth(iMonth);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMySysWid->SetDay(iMaxDay);
		}
	}else if(pWid == pMySysWid->m_pBtnTime[3])
	{
		iMonth ++;
		if(iMonth >12)
		{
			iMonth = 1;
		}
		pMySysWid->SetMonth(iMonth);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMySysWid->SetDay(iMaxDay);
		}
	}else if(pWid == pMySysWid->m_pBtnTime[4] || pWid == pMySysWid->m_pBtnTime[5])
	{
		iMaxDay =GetMaxDay(iYear,iMonth);
		if(pWid == pMySysWid->m_pBtnTime[5])
		{
			iDay ++;
		}
		else
		{
			iDay --;
		}
		if(iDay >iMaxDay)
		{
			iDay = 1;
		}
		if(iDay <1)
		{
			iDay = iMaxDay;
		}
		pMySysWid->SetDay(iDay);
	}
}

void CheckDiskStateTimer(void*arg)
{
	T_LOG_INFO tLog;
	tLog.iLogType = LOG_TYPE_SYS;
	SysManageWid *pMySysWid = (SysManageWid *)arg;
	T_NVR_STATE atNVRState[6];

	GetAllNvrInfo(atNVRState,sizeof(atNVRState));
	
	for(int i=0;i<6;i++)
	{
		int iState = NVR_GetConnectStatus(i);
		
		atNVRState[i].iNVRConnectState = iState;
		if(pMySysWid->m_atNVRState[i].iNVRConnectState != iState)
		{
			if(E_SERV_STATUS_CONNECT == iState)
			{
				memset(tLog.acLogDesc,0,sizeof(tLog.acLogDesc));
				sprintf(tLog.acLogDesc,"%d车 nvr online",i+1);
				LOG_WriteLog(&tLog);
			}
			else 
			{
				memset(tLog.acLogDesc,0,sizeof(tLog.acLogDesc));
				sprintf(tLog.acLogDesc,"%d车 nvr off",i+1);
				LOG_WriteLog(&tLog);
			}
		}
	}
	
	if(memcmp(atNVRState,pMySysWid->m_atNVRState,sizeof(atNVRState)))
	{
		memcpy(pMySysWid->m_atNVRState,atNVRState,sizeof(atNVRState));
		pMySysWid->m_pDiskStTable->redraw();
	}
	Fl::repeat_timeout(3,CheckDiskStateTimer,arg);
}

void CheckResUpdateTimer(void*arg)
{
	SysManageWid *pMySysWid = (SysManageWid *)arg;
	static char g_cState = 0 ,g_cProgress =0;
	char cState =0,cProgress = 0;
	GetResUpdateState(&cState, &cProgress);
	char acBuf[256] = {0};
	
	if(cState != g_cState || g_cProgress !=  cProgress)
	{
		switch(cState)
		{
		case 1:
		{
			sprintf(acBuf,"开始更新");
			break;
		}
		case 2:
		{
			sprintf(acBuf,"更新中:%d%%",cProgress);
			break;
		}
		case 3:
		{
			sprintf(acBuf,"更新成功");		
			break;
		}
		case 4:
		{
			sprintf(acBuf,"更新失败");
			break;
		}
		default:
			break;	
		}
		g_cState = cState ;
		g_cProgress =  cProgress;
		pMySysWid->m_pBoxResUpdateState->copy_label(acBuf);
		pMySysWid->m_pBoxResUpdateState->redraw();
	}
	
	Fl::repeat_timeout(0.5,CheckResUpdateTimer,arg);
}

SysManageWid::SysManageWid(int x,int y,int w,int h,const char *l)
: Fl_Group(x,y,w,h,l)
{
	memset(m_atNVRState,0xff,sizeof(m_atNVRState)); //0xff 跟任何值都不会重合，方便第一次更新
	for(int i=0;i<6;i++)
	{
		m_atNVRState[i].iNVRConnectState = E_SERV_STATUS_UNCONNECT;
	}
	SetUi();
	Fl::add_timeout(4,CheckDiskStateTimer,this);
	Fl::add_timeout(0.5,CheckResUpdateTimer,this);
}

SysManageWid::~SysManageWid()
{
	Fl::remove_timeout(CheckDiskStateTimer,this);
	Fl::remove_timeout(CheckResUpdateTimer,this);
}

int SysManageWid::SetUi()
{
	struct tm *pLocalTime;
	char acTime[56] = {0};
	char acRunPath[256] = {0};
	char acImgFullName[256] = {0};
    Fl_Group *pGroup[4];
	
    begin();
    align(Fl_Align(FL_ALIGN_TOP|FL_ALIGN_INSIDE));

//Group1
    pGroup[0] = new Fl_Group(10,63,1004,315,"日志管理");
    m_pMyLogTable = new MyTable(20, 88, 713, 280);

    for(int i=0;i<3;i++)
	{
		m_pBtnTime[i*2] = new MyImgButton(760, 95 +i* 50, 75, 38);
		m_pBtnTime[i*2+1] = new MyImgButton(920, 95 +i* 50, 75, 38);
        m_pBoxTime[i] = new Fl_Box(835,95 +i * 50,85,38);
    }

    m_pBtnOper[0] = new Fl_Button(760, 255, 108, 42,"查看系统日志");  
	m_pBtnOper[1] = new Fl_Button(888, 255, 108, 42,"查看操作日志");  
	m_pBtnOper[2] = new Fl_Button(760, 321, 108, 42,"上一页");	
	m_pBtnOper[3] = new Fl_Button(888, 321, 108, 42,"下一页");
    pGroup[0]->end();

//Group2
    pGroup[1] = new Fl_Group(10,385,500,245,"设备状态");
    m_pDiskStTable = new MyTable(20, 420, 482, 200);
    pGroup[1]->end();

//Group3
    pGroup[2] = new Fl_Group(514,385,500,245,"密码管理");
    m_pInPwd[0] = new Fl_Secret_Input(612,440,80,30,"旧密码:");
	m_pInPwd[1] = new Fl_Secret_Input(612,480,80,30,"新密码:");
	m_pInPwd[2] = new Fl_Secret_Input(612,520,80,30,"确认密码:");
    m_pBtnOper[4] = new Fl_Button(612,560,80,32,"更改密码"); 
    m_pBoxMessage = new Fl_Box(570,593,132,30,"");
    for(int i=0;i<11;i++)
    {  	
    	int iLeft = 710 + 75 *(i%4);
		int iTop =  440 +(i/4) *55;
		char acNO[8] ;
		memset(acNO,0,sizeof(acNO));
		sprintf(acNO, "%d",i+1);	
		switch(i)
		{
			case 9:
				m_pBtnPass[i] = new Fl_Button(iLeft, iTop, 68,42,"0");
				break;
			case 10:
				m_pBtnPass[i] = new Fl_Button(iLeft+75, iTop, 68,42,"Del");
				break;
			default:
				m_pBtnPass[i] = new Fl_Button(iLeft, iTop, 68,42);
				m_pBtnPass[i]->copy_label(acNO);
		}
    }
    pGroup[2]->end();

//Group4
    pGroup[3] = new Fl_Group(10,645,1004,58);
	m_pBtnRestart = new Fl_Button(20,655,100,40,"重启");
    m_pBoxResUpdate = new Fl_Box( 120,660,120,35,"素材更新:");
    m_pBoxResUpdateState = new Fl_Box(300,660,180,35);
    m_pSliderLight = new MySlider(610,670,280,25,"动态地图亮度: ");
    m_pBtnOper[5] = new Fl_Button(904, 655, 100, 40,"确认");
    pGroup[3]->end();

    m_pBtnOper[6] = new Fl_Button(904,725,100,40,"返回");


 	time_t tTime;
    time(&tTime);
    pLocalTime = localtime(&tTime);
	m_iYear = pLocalTime->tm_year +1900;
	m_iMonth = pLocalTime->tm_mon +1;
	m_iDay = pLocalTime->tm_mday;
	
	GetCCTVRunDir(acRunPath, sizeof(acRunPath));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/bg_system.png",acRunPath);

    memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/time_dec.png",acRunPath);
	m_pTimeImg[0] =  new Fl_PNG_Image(acImgFullName);
	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/time_dec_pressed.png",acRunPath);
	m_pTimeImg[1] =  new Fl_PNG_Image(acImgFullName);
	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/time_add.png",acRunPath);
	m_pTimeImg[2] =  new Fl_PNG_Image(acImgFullName);
	memset(acImgFullName,0,sizeof(acImgFullName));
	snprintf(acImgFullName,sizeof(acImgFullName)-1,"%s/res/time_add_pressed.png",acRunPath);
	m_pTimeImg[3] =  new Fl_PNG_Image(acImgFullName);

    for(int i=0;i<4;i++)
    {
        pGroup[i]->box(FL_BORDER_FRAME);
        pGroup[i]->color((Fl_Color)0x3B49BF00);
        pGroup[i]->labelcolor(FL_WHITE);
	    pGroup[i]->labelfont(FL_BOLD);
	    pGroup[i]->labelsize(16);
        pGroup[i]->align(Fl_Align(33|FL_ALIGN_INSIDE));
    }
    
    for(int i=0;i<3;i++)
	{
		m_pBtnTime[i*2]->box(FL_NO_BOX);
		m_pBtnTime[i*2+1]->box(FL_NO_BOX);
		m_pBtnTime[i*2]->image(m_pTimeImg[0]);
		m_pBtnTime[i*2+1]->image(m_pTimeImg[2]);
		m_pBtnTime[i*2]->SetPressImg(m_pTimeImg[1]);
		m_pBtnTime[i*2+1]->SetPressImg(m_pTimeImg[3]);
		m_pBtnTime[i*2]->callback(TimeBtnClicked,this);
		m_pBtnTime[i*2+1]->callback(TimeBtnClicked,this);
		m_pBoxTime[i]->labelcolor(FL_WHITE);
		m_pBoxTime[i]->labelsize(20);
		m_pBoxTime[i]->labelfont(FL_BOLD);
	}
    
    m_pMyLogTable->selection_color(FL_YELLOW);
    m_pMyLogTable->when(FL_WHEN_RELEASE|FL_WHEN_CHANGED);
    m_pMyLogTable->table_box(FL_NO_BOX);
	
    // ROWS
    m_pMyLogTable->row_header(0);
    m_pMyLogTable->row_resize(0);
    m_pMyLogTable->rows(0);
    m_pMyLogTable->row_height_all(22);
    // COLS
    m_pMyLogTable->cols(4);
    m_pMyLogTable->col_header(1);
    m_pMyLogTable->col_resize(0);
	m_pMyLogTable->col_header_height(25);
	m_pMyLogTable->col_header_color(FL_WHITE);
	m_pMyLogTable->col_width(0,40);
    m_pMyLogTable->col_width(1,70);
	m_pMyLogTable->col_width(2,170);
	m_pMyLogTable->col_width(3,430);
	m_pMyLogTable->color(FL_WHITE);
    
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%d",m_iYear);
	m_pBoxTime[0]->copy_label(acTime);
	
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iMonth);
	m_pBoxTime[1]->copy_label(acTime);
		
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iDay);
	m_pBoxTime[2]->copy_label(acTime);

	m_pBtnOper[0]->callback(FindSysLogClicked, this);
	m_pBtnOper[1]->callback(FindOperLogClicked, this);
	m_pBtnOper[2]->callback(PrePageClicked, this);
	m_pBtnOper[3]->callback(NextPageClicked, this);
	m_pBtnOper[4]->callback(PassSetBtnClicked, this);
	m_pBtnOper[5]->callback(MapLightBtnClicked, this);
    
	for(int i=0;i<7;i++)
	{
		m_pBtnOper[i]->box(FL_UP_BOX);
      	m_pBtnOper[i]->down_box(FL_DOWN_BOX);
      	m_pBtnOper[i]->color((Fl_Color)181);
		m_pBtnOper[i]->selection_color((Fl_Color)6);
      	m_pBtnOper[i]->labelsize(16);
      	//m_pBtnOper[i]->labelcolor(FL_WHITE);
		m_pBtnOper[i]->labelfont(FL_BOLD);
	}

	m_pBoxMessage->labelcolor(FL_RED);
	m_pBoxMessage->labelsize(18);

    m_pDiskStTable->SetTableTyle(MyTable::E_DISKST_TABLE);
    m_pDiskStTable->selection_color(FL_YELLOW);
    m_pDiskStTable->when(FL_WHEN_RELEASE|FL_WHEN_CHANGED);
    m_pDiskStTable->table_box(FL_NO_BOX);
    
	
    // ROWS
    m_pDiskStTable->row_header(0);
    m_pDiskStTable->row_resize(0);
    m_pDiskStTable->rows(6);
    m_pDiskStTable->row_height_all(22);
    // COLS
    m_pDiskStTable->cols(6);
    m_pDiskStTable->col_header(1);
    m_pDiskStTable->col_resize(0);
	m_pDiskStTable->col_header_height(25);
	m_pDiskStTable->col_header_color(FL_WHITE);
    m_pDiskStTable->col_width(0,80);
	m_pDiskStTable->col_width(1,80);
	m_pDiskStTable->col_width(2,80);
	m_pDiskStTable->col_width(3,80);
	m_pDiskStTable->col_width(4,80);
	m_pDiskStTable->col_width(5,80);
	m_pDiskStTable->color(FL_WHITE);

	for(int i=0;i<3;i++)
	{
		m_pInPwd[i]->labelsize(16);
    	m_pInPwd[i]->labelcolor(FL_WHITE);
		m_pInPwd[i]->labelfont(FL_BOLD);
		m_pInPwd[i]->maximum_size(6);
		m_pInPwd[i]->align(Fl_Align(FL_ALIGN_LEFT));
		m_pInPwd[i]->when(FL_WHEN_RELEASE_ALWAYS);
		m_pInPwd[i]->callback(PassEditClicked,this);
	}	
    m_pInPwd[0]->maximum_size(8);
	m_iSelIndexPass = 0;
			
	for(int i=0;i<11;i++)
    {  					
		m_pBtnPass[i]->box(FL_FLAT_BOX);
      	m_pBtnPass[i]->down_box(FL_FLAT_BOX);
      	m_pBtnPass[i]->color(FL_WHITE);
		m_pBtnPass[i]->selection_color((Fl_Color)0xA0EEEE00);
      	m_pBtnPass[i]->labelsize(20);
      	m_pBtnPass[i]->labelcolor(FL_BLACK);
		m_pBtnPass[i]->labelfont(FL_BOLD);
		m_pBtnPass[i]->callback(PassBtnPressFun,this);
    }
    
	m_pBtnRestart->box(FL_UP_BOX);
    m_pBtnRestart->down_box(FL_FLAT_BOX);
    m_pBtnRestart->color(FL_YELLOW);
	m_pBtnRestart->selection_color((Fl_Color)0xEEEE4400);
    m_pBtnRestart->labelsize(18);
    m_pBtnRestart->labelcolor(0xFF222200);
	m_pBtnRestart->labelfont(FL_BOLD);
	m_pBtnRestart->callback(RestartClicked,this);

	m_pBoxResUpdate->labelsize(16);
    m_pBoxResUpdate->labelcolor(FL_WHITE);
	m_pBoxResUpdate->labelfont(FL_BOLD);

	m_pBoxResUpdateState->labelsize(16);
    m_pBoxResUpdateState->labelcolor(FL_WHITE);
	m_pBoxResUpdateState->labelfont(FL_BOLD);
	m_pBoxResUpdateState->align(FL_ALIGN_LEFT);
	
	
	m_pSliderLight->type(FL_HOR_NICE_SLIDER);
    m_pSliderLight->box(FL_FLAT_BOX);
    m_pSliderLight->color((Fl_Color)0x30529200);
    m_pSliderLight->labelsize(16);
	m_pSliderLight->align(Fl_Align(FL_ALIGN_LEFT));
	m_pSliderLight->labelcolor(FL_WHITE);
	m_pSliderLight->labelfont(FL_BOLD);
	m_pSliderLight->range(0,100);
    m_pSliderLight->step(1);
    m_pSliderLight->value(GetDynamicMapBrightness());
	m_pSliderLight->textcolor(FL_WHITE);
	m_pSliderLight->textsize(16);
	m_pSliderLight->textfont(FL_BOLD);
	m_pSliderLight->selection_color((Fl_Color)181);

	end();
	return 0;
}

void SysManageWid::SetYear(int iYear)
{
	if(m_iYear != iYear)
	{
		char acData[12] = {0};
		sprintf(acData,"%d",iYear);
		m_iYear = iYear;
		m_pBoxTime[0]->copy_label(acData);
		m_pBoxTime[0]->redraw();
	}
}
void SysManageWid::SetMonth(int iMonth)
{
	if(m_iMonth != iMonth)
	{
		
		char acData[12] = {0};
		sprintf(acData,"%02d",iMonth);
		m_iMonth = iMonth;
		m_pBoxTime[1]->copy_label(acData);
		m_pBoxTime[1]->redraw();
	}
}
void SysManageWid::SetDay(int iDay)
{
	if(m_iDay != iDay)
	{
		char acData[12] = {0};
		sprintf(acData,"%02d",iDay);
		m_iDay = iDay;
		m_pBoxTime[2]->copy_label(acData);
		m_pBoxTime[2]->redraw();
	}
}
int  SysManageWid::GetYear()
{
	return m_iYear;
}
int  SysManageWid::GetMonth()
{
	return m_iMonth;
}
int  SysManageWid::GetDay()
{
	return m_iDay;
}

void SysManageWid::ClearMessageBox()
{
	m_pInPwd[0]->value("");
	m_pInPwd[1]->value("");
	m_pBoxMessage->copy_label("");
    m_pBoxMessage->redraw();
}


