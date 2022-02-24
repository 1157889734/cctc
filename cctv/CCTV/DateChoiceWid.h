#ifndef _DATE_CHOICE_H_
#define _DATE_CHOICE_H_

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_PNG_Image.H>

#include "MyTable.h"
#include "MySlider.h"
#include "MyImgButton.h"



typedef void (DateCBFun)(Fl_Widget *widget,void *pData);

class FL_EXPORT DateChoiceWid : public Fl_Double_Window{

protected:
	int handle(int);
public:
	DateChoiceWid(int x,int y,int w,int h,const char *l=0);
	virtual ~DateChoiceWid();
	int GetResult();
	void SetHideCBFun(DateCBFun *pFun);
	void SetDataPtr(void *pData);
	void GetTimeStr(char *pStrDate);
	int  GetYear();
	int  GetMonth();
	int  GetDay();
	int  GetHour();
	int  GetMin();
	int  GetSec();

	void  SetYear(int iYear);
	void  SetMonth(int iMouth);
	void  SetDay(int iDay);
	void  SetHour(int iHour);
	void  SetMin(int iMin);
	void  SetSec(int iSec);
	
	Fl_Box	  *m_pBoxTime[6];
	Fl_Button *m_pBtnTime[12];
	Fl_Button *m_pBtnOper[2];
	Fl_Box	  *m_pBoxTitle[2];
	int m_iResult;
private:
	
	DateCBFun *m_pDateCBFun;
	void  *m_pData;	
	int m_iYear;
	int m_iMonth;
	int m_iDay;
	int m_iHour;
	int m_iMin;
	int m_iSec;

};

#endif

