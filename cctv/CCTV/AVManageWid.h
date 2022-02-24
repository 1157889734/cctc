#ifndef _AV_MANAGE_H_
#define _AV_MANAGE_H_

#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include "MySlider.h"
#include <FL/Fl_Box.H>



typedef void (AVManaCBFun)(Fl_Widget *widget,void *pData);


class FL_EXPORT AVManageWid : public Fl_Double_Window{

private:
	int SetUi();
public:
	~AVManageWid();
	AVManageWid(int x,int y,int w, int h,const char* l=0);
	void SetDataPtr(void *pData);
	void UpdateState();
	void  *m_pData;	

	Fl_PNG_Image *m_pBgImg;	
	Fl_Box *m_pBoxBg;
	Fl_Group *m_pRdGroup;
	Fl_Round_Button *m_pRdBtn[2];
	Fl_Check_Button *m_pCheckBtn[2];
	Fl_Button *m_pBtn[5];
	MySlider   *m_pMySlider[3];
	Fl_Box 	   *m_pBoxVol[2];
	unsigned char	m_i8VolMoni;
	unsigned char 	m_i8VolTrain;	
	int				m_iSrcMode;
	Fl_Box	   	   *m_pBoxMessage;
};




#endif

