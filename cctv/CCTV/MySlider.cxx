#include "MySlider.h"
#include <FL/Fl.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/fl_draw.H>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

MySlider::MySlider(int x,int y,int w,int h, const char *l)
: Fl_Value_Slider(x,y,w,h,l) {
  m_eType = E_NORMAL;
}

void MySlider::Fl_Slider_draw(int X, int Y, int W, int H) {

  double val;
  if (minimum() == maximum())
    val = 0;
  else {
    val = (value()-minimum())/(maximum()-minimum());
    if (val > 1.0) val = 1.0;
    else if (val < 0.0) val = 0.0;
  }

  int ww = (horizontal() ? W : H);
  int xx, S;
  if (type()==FL_HOR_FILL_SLIDER || type() == FL_VERT_FILL_SLIDER) {
    S = int(val*ww+.5);
    if (minimum()>maximum()) {S = ww-S; xx = ww-S;}
    else xx = 0;
  } else {
    S = 10;
    xx = int(val*(ww-S)+.5);
  }
  int xsl, ysl, wsl, hsl;  
  if (horizontal()) {
    
    xsl = X+xx;
    wsl = S;
    ysl = Y+2;
    hsl = H-4;
  } else {
    ysl = Y+xx;
    hsl = S;
    xsl = X+2;
    wsl = W-4;
  }

  //draw_bg(X, Y, W, H);
  {
  	fl_push_clip(X, Y, W, H);
  	draw_box();
  	fl_pop_clip();

  	Fl_Color black = active_r() ? FL_WHITE : FL_INACTIVE_COLOR;
  	if (type() == FL_VERT_NICE_SLIDER) {
    draw_box(FL_THIN_DOWN_BOX, X+W/2-2, Y, 4, H, black);
  	} else if (type() == FL_HOR_NICE_SLIDER) {
  	draw_box(FL_THIN_DOWN_BOX, X, Y+H/2-2, xx, 4, selection_color());
    draw_box(FL_THIN_DOWN_BOX, xsl, Y+H/2-2, W-xx, 4, black);
  	}
  }

  Fl_Boxtype box1 = slider();
  if (!box1) {box1 = (Fl_Boxtype)(box()&-2); if (!box1) box1 = FL_UP_BOX;}
  if (type() == FL_VERT_NICE_SLIDER) {
    draw_box(box1, xsl, ysl, wsl, hsl, FL_WHITE);
    int d = (hsl-4)/2;
    draw_box(FL_THIN_DOWN_BOX, xsl+2, ysl+d, wsl-4, hsl-2*d,selection_color());
  } else if (type() == FL_HOR_NICE_SLIDER) {
    draw_box(box1, xsl, ysl, wsl, hsl, FL_WHITE);
    //int d = (wsl-4)/2;
    //draw_box(FL_THIN_DOWN_BOX, xsl+d, ysl+2, wsl-2*d, hsl-4,selection_color());
  } else {
    if (wsl>0 && hsl>0) draw_box(box1, xsl, ysl, wsl, hsl, selection_color());
  	}
  draw_label(xsl, ysl, wsl, hsl);
  if (Fl::focus() == this) {
    if (type() == FL_HOR_FILL_SLIDER || type() == FL_VERT_FILL_SLIDER) draw_focus();
    else draw_focus(box1, xsl, ysl, wsl, hsl);
  }
}


void MySlider::draw()
{
  int sxx = x(), syy = y(), sww = w(), shh = h();
  int bxx = x(), byy = y(), bww = w(), bhh = h();
  bww =60;
  bhh = 25;
  
  if(m_eType == E_PROGRESS)
  {
  		bww = 120;
    	bhh = 25;
  }
  if (horizontal()) {
   		bxx = sxx + sww -bww; 
   		sww -= bww ;
  } else {
    	byy = syy + shh -bhh; 
    	syy -= bhh;
  }
  if (damage()&FL_DAMAGE_ALL)
  {
  	draw_box(box(),sxx,syy,sww,shh,color());
  }
  	
  Fl_Slider_draw(sxx+Fl::box_dx(box()),
		  syy+Fl::box_dy(box()),
		  sww-Fl::box_dw(box()),
		  shh-Fl::box_dh(box()));
  char buf[256] = {0};
  char acTime[128] = {0};
  format(buf);
  if(m_eType == E_PROGRESS)
  {
  	int iTime = atoi(buf);
	int iMax = maximum();
	sprintf(acTime,"%02d:%02d/%02d:%02d",(iTime)/60,iTime%60
		,iMax/60,iMax%60);
  }
  draw_box(box(),bxx,byy,bww,bhh,color());
  fl_font(textfont(), textsize());
  fl_color(active_r() ? textcolor() : fl_inactive(textcolor()));
  if(m_eType == E_PROGRESS)
  {
  	fl_draw(acTime, bxx, byy, bww, bhh, FL_ALIGN_CLIP);
  }
  else
  {
  	fl_draw(buf, bxx, byy, bww, bhh, FL_ALIGN_CLIP);
  }
}

void MySlider::SetSliderType(Fl_SliderType eType )
{
	m_eType = eType;
}

