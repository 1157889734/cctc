#ifndef CCTV_WID_H
#define CCTV_WID_H

#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_BMP_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include "MoniManageWid.h"
#include "MyPlayBoxWid.h"
#include "AVManageWid.h"
#include "pmsgcli.h"
#include "CMPlayer.h"


int  InitCCTV(void *arg);
int  StartNtp();

class FL_EXPORT My_CCTV_Window : public Fl_Double_Window 
{

public:	
	
  ~My_CCTV_Window();
  My_CCTV_Window(int X, int Y, int W, int H, const char *l = 0);
  int HideCCTVResp();
  
  int ShowCCTVResp();
  
  int 				SetUi();

  MoniManageWid    *m_pMoniMgeWid;  //监控管理界面
  AVManageWid 	   *m_pAVWid;       //影音管理界面
  PasswordWid 	   *m_pPassWid ;	//密码确认界面
  unsigned short 	m_u16Year;	
  unsigned char  	m_u8Mon;
  unsigned char	    m_u8Day;
  int			  	m_iWeek;
  Fl_Box 		   *m_pBoxDate[3];  //日期 星期 时间列表

  My_Play_Box 	   *m_p4BgPlay[4];  //四画面播放时的界面
  My_Play_Box 	   *m_pSinglePlayBg;//单画面播放时的界面

  Fl_Button		   *m_pBtn1[4];     //监控管理 影音管理 HMI MMI
  Fl_Button		   *m_pBtn2[4];     //单画面 四画面 轮巡开 轮巡关
  Fl_PNG_Image 	   *m_pImgBtn1[8];  //单画面 四画面 轮巡开 轮巡关的背景图
  Fl_Button		   *m_pBtn3[8][4];  //底下的32个按钮
  
  //0:在线播放中 1:不在线播放中 2:在线未播放 3:不在线未播放 4：遮挡报警
  Fl_PNG_Image 	   *m_pImgBtn2[32][5]; 
 
  Fl_Box 			*m_pBoxBk1;		//右边的背景
  Fl_Box 			*m_pBoxBk2;		//底下的背景
  Fl_PNG_Image 		*m_pImgBk1;		//右边背景图片
  Fl_PNG_Image 		*m_pImgBk2;		//底下背景图片
  time_t 			m_tLastTime;		//播放按钮点击的最后时间  避免视频切换太快	
  struct timeval    m_tPrevClickTime;   //全屏切换上次点击的时间 避免全屏非全屏切换太快
  char				m_acFireWarnInfo[6];	//烟火报警的值
  char				m_acDoorWarnInfo[6];	//门禁报警的值
  char              m_acDoorClipWarnInfo[6];//门夹报警的值
  unsigned int		m_iPecuInfo;		//Pecu报警的值
  
  Fl_Button		   *m_pBoxFire[6];      //烟火报警的图标
  Fl_Button		   *m_pBoxDoor[24];	    //门禁报警的图标
  Fl_Button		   *m_pBoxDoorClip[24];	//门夹报警的图标
  Fl_Button		   *m_pBoxPecu[24];		//PECU报警的图标
  int				m_iFireCount;		//门禁报警的数量
  int				m_iDoorCount;		//烟火报警的数量
  int				m_iDoorClipCount;	//门卡报警的数量
  int				m_iPecuCount;		//Pecu报警的数量
  int				m_aiFireIdx[6];		//门禁报警的相机序号
  int				m_aiDoorIdx[24];		//烟火报警的相机序号
  int				m_aiPecuIdx[24];		//Pecu报警的相机序号
  int               m_aiDoorClipIdx[24]; //门卡报警的相机序号
  Fl_BMP_Image 	   *m_pImgFire;
  Fl_BMP_Image 	   *m_pImgDoor;
  Fl_BMP_Image     *m_pImgDoorClick;
  Fl_BMP_Image 	   *m_pImgPecu;

  
};

#endif

