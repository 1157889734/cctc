#ifndef SYS_MANAGE_WID_H
#define SYS_MANAGE_WID_H

#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_PNG_Image.H>
#include "MyTable.h"
#include "MySlider.h"
#include "MyImgButton.h"
#include "state.h"
#include "PasswordWid.h" 

class FL_EXPORT SysManageWid : public Fl_Group{

public:
	SysManageWid(int x,int y,int w,int h,const char *l=0);
	~SysManageWid();
	void SetYear(int iYear);
	void SetMonth(int iMonth);
	void SetDay(int iDay);
	int  GetYear();
	int  GetMonth();
	int  GetDay();
	
	Fl_Box *m_pBoxSys;
	Fl_Box *m_pBoxDiskState;
	Fl_Box *m_pBoxPassMana;
	Fl_Box *m_pBoxNewPass;
	MyTable *m_pMyLogTable;
	MyTable *m_pDiskStTable;
	Fl_PNG_Image *m_pImgBk;
	MyImgButton *m_pBtnTime[6];
	Fl_Box	  *m_pBoxTime[3];
	Fl_PNG_Image *m_pTimeImg[4];
	Fl_Button *m_pBtnOper[7];
	Fl_Button *m_pBtnPass[11];
	Fl_Secret_Input  *m_pInPwd[3];
	Fl_Button *m_pBtnRestart;
	Fl_Box	  *m_pBoxResUpdate;
	Fl_Box	  *m_pBoxResUpdateState;
	MySlider  *m_pSliderLight;
	int		   m_iSelIndexPass;
	T_NVR_STATE m_atNVRState[6];	
	Fl_Box	   	*m_pBoxMessage;
	void        ClearMessageBox();
private:
	int 	   SetUi();
	int		   m_iYear;
	int		   m_iMonth;
	int		   m_iDay;

};
#endif
