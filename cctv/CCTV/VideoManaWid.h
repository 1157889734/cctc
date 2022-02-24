#ifndef _VIDEO_MANA_WID_H_
#define _VIDEO_MANA_WID_H_


#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include "MyTable.h"
#include "MySlider.h"
#include "pmsgcli.h"
#include "CMPlayer.h"
#include "DateChoiceWid.h"


class FL_EXPORT VideoManageWid : public Fl_Group{

public:
	VideoManageWid(int x,int y,int w,int h,const char *l=0);
	~VideoManageWid();
	int  BeginTimer();
	int  EndTimer();
	Fl_Button *m_pBtnTime[2];
	Fl_Box	  *m_pBoxTimeTitle[3];
	Fl_Box	  *m_pBoxDate[2];
	Fl_Box	  *m_pBoxSpeed;
	int m_iSelIdxTime;
	T_TIME_INFO m_tStartTime;
	T_TIME_INFO m_tEndTime;
	Fl_Choice  *m_pChocie[3];
	Fl_Button  *m_pBtnOper[10];
	MyTable    *m_pFilelsTable;
	Fl_Box	   *m_pBoxPlayWid;
	MySlider   *m_pMyPlaySlider;
	int		    m_iNvrNo;
	int		    m_iWaitFileCnt;
	Fl_Box     *m_pBtnProgress[2];
	vector<int> m_VecSelFile;
	double		m_dbSpeed;
	int			m_iTotolTime;
	int			m_iTimeCount;
	CMPHandle   m_pHplay;
	Fl_Box	   *m_pBoxMessage;
	int 		closePlayVideo();
	DateChoiceWid *m_pDateChoiceWid;
};


#endif

