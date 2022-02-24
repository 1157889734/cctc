#ifndef MONIMANAGE_WID_H
#define MONIMANAGE_WID_H

#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include "SysManageWid.h"
#include "VideoManaWid.h"


typedef void (BackCBFun)(Fl_Widget *widget,void *pData);

class FL_EXPORT MoniManageWid : public Fl_Double_Window{

private:
	int SetUi();
	
public:
	Fl_Button* 		 m_pBtn1[2];
	//Fl_Box 	  *m_pBoxBk[2];

	SysManageWid 	*m_pSysManaWid;
	VideoManageWid	*m_pVidManaWid;
	
	Fl_Window	 *m_pCCTVWid;
    BackCBFun    *m_pBackCBFun;
    void *m_pData;
	
	~MoniManageWid(){};
	MoniManageWid(int x,int y,int w, int h,const char* l=0);
    void SetBackCBFun(BackCBFun *pBackFun,void *pData);
	
};

#endif