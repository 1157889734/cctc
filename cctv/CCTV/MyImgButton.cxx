#include "MyImgButton.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <string.h>


MyImgButton::MyImgButton(int x,int y,int w,int h, const char *l)
: Fl_Button(x,y,w,h,l) {
  m_pPressImg = NULL;
}


void MyImgButton::draw()
{
  if (type() == FL_HIDDEN_BUTTON) return;
  Fl_Color col = value()? selection_color() : color();
  Fl_Boxtype iBoxType = value() ? (down_box()?down_box():fl_down(box())) : box();
  if(iBoxType)
  {
  	draw_box(iBoxType, x(), y(), w(), h(), col);
  }
  

  // if (align() & FL_ALIGN_IMAGE_BACKDROP) 
  {
   	const Fl_Image *img = value() ? (m_pPressImg?m_pPressImg:image()):image();
    
    if (img && deimage() && !active_r())
      img = deimage();
    if (img)
      ((Fl_Image*)img)->draw(x()+(w()-img->w())/2, y()+(h()-img->h())/2);
  }
 
  if (Fl::focus() == this) draw_focus();
}

void MyImgButton::SetPressImg(Fl_Image *img)
{
	m_pPressImg = img;
}

