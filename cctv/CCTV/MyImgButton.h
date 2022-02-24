#ifndef MY_IMG_BUTTON_H
#define MY_IMG_BUTTON_H


#include <FL/Fl_Button.H>

class FL_EXPORT MyImgButton : public Fl_Button {

public:
	MyImgButton(int x,int y,int w,int h, const char *l = 0);
	void SetPressImg(Fl_Image *img);
protected:
    void draw();
	Fl_Image *m_pPressImg;
};

#endif

