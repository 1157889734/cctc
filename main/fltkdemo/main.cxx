#include <stdio.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <asm/mman.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
Fl_Button *g_pBtnLefTop=NULL;
Fl_Button *g_pBtnLefBottom=NULL;
Fl_Button *g_pBtnRightTop=NULL;
Fl_Button *g_pBtnRightBottom=NULL;
Fl_Button *g_pBtnCenter=NULL;
Fl_Button *g_pBtnExit=NULL;
Fl_Box 	  *g_pBox = NULL;

void BtnClicked(Fl_Widget*pBtn,void *pData)
{
	if(pBtn == g_pBtnLefTop)
	{
		g_pBox->copy_label("Pressed F1");
	}else if(pBtn == g_pBtnLefBottom)
	{
		g_pBox->copy_label("Pressed F2");
	}else if(pBtn == g_pBtnRightTop)
	{
		g_pBox->copy_label("Pressed F3");
	}else if(pBtn == g_pBtnRightBottom)
	{
		g_pBox->copy_label("Pressed F4");
	}else if(pBtn == g_pBtnCenter)
	{
		g_pBox->copy_label("Pressed F5");
	}else if(pBtn == g_pBtnExit)
	{
		system("./openTestApp.sh -qws &");
		exit(0);
	}

}
int main()
{
	Fl_Double_Window *pCCTVWid = new Fl_Double_Window(0,0,1024,768);
	g_pBtnLefTop = new Fl_Button(10,10,100,40,"F1");
	g_pBtnLefBottom= new Fl_Button(10,700,100,40,"F2");
	g_pBtnRightTop = new Fl_Button(900,10,100,40,"F3");
	g_pBtnRightBottom= new Fl_Button(900,700,100,40,"F4");
	g_pBtnCenter= new Fl_Button(462,364,100,40,"F5");
	g_pBtnExit= new Fl_Button(462,10,100,40,"Test");
	g_pBox = new Fl_Box(412, 500, 200, 24);
	g_pBox->labelsize(20);

	g_pBtnLefTop->callback(BtnClicked,pCCTVWid);
	g_pBtnLefBottom->callback(BtnClicked,pCCTVWid);
	g_pBtnRightTop->callback(BtnClicked,pCCTVWid);
	g_pBtnRightBottom->callback(BtnClicked,pCCTVWid);
	g_pBtnCenter->callback(BtnClicked,pCCTVWid);
	g_pBtnExit->callback(BtnClicked,pCCTVWid);
	pCCTVWid->end();
	pCCTVWid->show();
	Fl::run();    
}
