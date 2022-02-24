#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "NBML2S.h"
#include "mvb_data_buf.h"
#include <FL/Fl_Button.H>
int g_argc;
char ** g_argv;
Fl_Double_Window *g_pCCTVWid =NULL;
Fl_Button *g_pBtnHMI=NULL;
Fl_Button *g_pBtnDMI=NULL;

extern void Subrail_Term_Judge();
extern void get_display_config();
extern unsigned int Display_Config;

extern void DMI_Life_Judge();
extern unsigned int G_DMI_Life_Flag;
extern void HMI_Life_Judge();
extern unsigned int G_HMI_Life_Flag;


/**
 * cctv监控dmi和hmi生命信号
 */
void SwitchWindowTimer(void *arg)
{
	data_buf.mvb_data_recv();

	DMI_Life_Judge();
	HMI_Life_Judge();
	if(G_DMI_Life_Flag!=1)
	{
		main_dmi(g_argc,g_argv);
		Fl::remove_timeout(SwitchWindowTimer);
		g_pCCTVWid->hide();		//dmi生命信号无效，切换至DMI
		return;
	}
	else
	{
		if(G_HMI_Life_Flag!=1)
		{
			main_hmi(g_argc,g_argv);
			Fl::remove_timeout(SwitchWindowTimer);
			g_pCCTVWid->hide();//dmi生命信号无效，切换至HMI
			return;
		}
	}
	Fl::repeat_timeout(0.3,SwitchWindowTimer,arg);
}	


static void ChangeWid(Fl_Widget*pWid,void *pData)
{
	Fl_Double_Window *pCCTVWid = (Fl_Double_Window *)pData;
	if(g_pBtnHMI == pWid )   //hmi
	{
		main_hmi(g_argc,g_argv);
		pCCTVWid->hide();
	}
	else
	{
		main_dmi(g_argc,g_argv);
		pCCTVWid->hide();
	}
}
void ShowCCTV(Fl_Widget*pWid,void *pData)
{
	g_pCCTVWid->show();
	g_pCCTVWid->cursor(FL_CURSOR_ARROW);
}
int main(int argc, char **argv)
{
	g_argc = argc;
	g_argv = argv;
	get_display_config();
	switch(Display_Config)
	{
		case 1:
			main_dmi(argc,argv);
			break;
		case 2:
			main_hmi(argc,argv);
			break;
		default:
			data_buf.init(0x35,"./pitcctv.csv"); //可以带参数指定设备地址
			Subrail_Term_Judge();
			g_pCCTVWid = new Fl_Double_Window(0,0,1024,768);
			g_pBtnHMI = new Fl_Button(200,200,100,40,"HMI");
			g_pBtnDMI= new Fl_Button(400,200,100,40,"MMI");
			g_pBtnHMI->callback(ChangeWid,g_pCCTVWid);
			g_pBtnDMI->callback(ChangeWid,g_pCCTVWid);
			set_cctv_show_cbfun(ShowCCTV);
			Fl::add_timeout(3,SwitchWindowTimer,g_pCCTVWid);
			g_pCCTVWid->show();
			g_pCCTVWid->cursor(FL_CURSOR_ARROW);
			break;
	}
	Fl::run();
	return 0;
}

