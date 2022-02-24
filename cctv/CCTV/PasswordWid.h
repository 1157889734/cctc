#ifndef PASSWORD_H
#define PASSWORD_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Secret_Input.H>

#define PASSWORDDIR "/mnt/confs"


typedef void (HideCBFun)(Fl_Widget *widget,void *pData);

class FL_EXPORT PasswordWid : public Fl_Double_Window{

public:
	PasswordWid(int x,int y,int w,int h,const char *l=0);
	virtual ~PasswordWid();
	int  GetResult();
	void SetHideCBFun(HideCBFun *pFun,void *pData);
    void SetTiTle(char *pText);
	void SetBoxTipVisible(int iShow);
	Fl_Button        *m_pBtnPass[13];
	Fl_Secret_Input  *m_pInPass;
	int               m_iResult;
	HideCBFun        *m_pHideCBFun;
	void  			 *m_pData;
	Fl_Box	   		 *m_pBoxMessage;
    Fl_Box	   		 *m_pBoxTiTle;
	Fl_Box	   		 *m_pBoxTip;

};

#endif
