#ifndef _MESSAGE_X_H
#define _MESSAGE_X_H

#include "FL/Enumerations.H"

void fl_message_x(const char *,...);
void fl_message_title_x(const char *title);
void fl_message_pos_x(int x, int y);
void fl_message_font_x(Fl_Font f, Fl_Fontsize s);


extern const char* fl_close_x;

#endif