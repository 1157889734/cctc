#ifndef MY_SLIDER_H
#define MY_SLIDER_H


#include <FL/Fl_Value_Slider.H>

class FL_EXPORT MySlider : public Fl_Value_Slider {


public:
	enum Fl_SliderType
	{
		E_NORMAL,
		E_PROGRESS,
	};
	MySlider(int x,int y,int w,int h, const char *l = 0);
	void SetSliderType(Fl_SliderType eType = E_NORMAL);
protected:
    void draw();
	void Fl_Slider_draw(int X, int Y, int W, int H) ;
	Fl_SliderType m_eType;
    Fl_Label *m_pLabel;
};

#endif

