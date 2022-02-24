#include "CCTV.h"
#include "CCTVWid.h"
#include "lmsgProc.h"
#include "state.h"
#include "msgapp.h"

static My_CCTV_Window *g_pCCTVWid = NULL;
static Fl_Double_Window *g_pWarnWid = NULL;
static Fl_Box		     *g_pBox = NULL;

int main_cctv(int argc, char *argv[])
{
	int iState = 0;
	char *ptr = strrchr(argv[0],'/');
	int iRet = 0;

    system("killall ntpcli.exe");
	usleep(1000);
	if(ptr)
	{
		SetCCTVRunDir(argv[0], ptr - argv[0]);
	}
	else
	{
		SetCCTVRunDir(".", 1);
	}
	
	iRet = STATE_Init();
	if(0 != iRet)
	{	
		g_pWarnWid = new Fl_Double_Window(0,0,1024,768);
		g_pBox = new Fl_Box(400, 200, 224, 24,strdup("CCTVConfig.ini配置错误"));
		g_pBox->labelsize(24);
		g_pWarnWid->end();	
		g_pWarnWid->show();

		return -1;
	}
	else
	{
		LMSG_Init();
    	LMSG_SendMsgToDHMI(MSG_CCTV2DHMI_ASYNC_REQUEST_STATE, NULL, 0);
    	// 休眠1秒钟，等待CCTV的当前显示状态
    	usleep(100000);
    	iState = GetDisplayState();
		InitCCTV(NULL);
		switch(iState)
    	{
       		case DISP_STATE_DMI:  
       		case DISP_STATE_HMI:
       		{
       			g_pCCTVWid = new My_CCTV_Window(0,0,1024,768);
				g_pCCTVWid->HideCCTVResp();
                g_pCCTVWid->hide();
				break;
       		}
	   		case DISP_STATE_CCTV:
       		default:
       		{
       			SetDisplayState(DISP_STATE_CCTV);
       	    	g_pCCTVWid = new My_CCTV_Window(0,0,1024,768);
                g_pCCTVWid->show();
                StartNtp();
                break;
       		}   
    	}
		
	}
	
	return 0;
}

int GetDevDispConfig()
{
	int iMode = GetDevDispMode();
	
	return iMode;
}