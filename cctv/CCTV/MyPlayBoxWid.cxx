#include "MyPlayBoxWid.h"
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>


My_Play_Box::My_Play_Box(int X, int Y, int W, int H, const char *l)
: Fl_Box(X,Y,W,H,l) 
{
}

int My_Play_Box::handle(int event) {
	switch(event)
	{
		case FL_ENTER:
		case FL_LEAVE:
			return 1;
		case FL_PUSH:
			return 1;
		case FL_RELEASE:
				do_callback();
			return 1;
		default:
			return 0;
	}
}

void My_Play_Box::draw() 
{
	fl_draw_box(FL_FLAT_BOX, x(), y(), w(), h(), FL_WHITE);
	fl_draw_box(FL_FLAT_BOX, x(), y(), w()-1, h()-1, color());
	draw_label();
}

