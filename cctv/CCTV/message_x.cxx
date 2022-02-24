//
// "$Id$"
//
// Message test program for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#include "message_x.h"

#include <stdio.h>
#include <stdarg.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/x.H>
#include <FL/fl_draw.H>

const char* fl_close_x= "Close"; 

static Fl_Window *message_form;
static Fl_Box *message;
static Fl_Box *icon;
static Fl_Button *button[3];
static Fl_Input *input;
static int ret_val;
static const char *iconlabel = "?";
static const char *message_title_default = "提示";
Fl_Font s_fl_message_font_x = FL_HELVETICA;
Fl_Fontsize s_fl_message_size_x = -1;
static int enableHotspot = 0;
static char avoidRecursion = 0;

// Sets the global return value (ret_val) and closes the window.
// Note: this is used for the button callbacks and the window
// callback (closing the window with the close button or menu).
// The first argument (Fl_Widget *) can either be an Fl_Button*
// pointer to one of the buttons or an Fl_Window* pointer to the
// message window (message_form).
static void button_cb_x(Fl_Widget *, long val) {
  ret_val = (int) val;
  message_form->hide();
}

static Fl_Window *makeform_x() {
 if (message_form) {
   return message_form;
 }
 // make sure that the dialog does not become the child of some
 // current group
 Fl_Group *previously_current_group = Fl_Group::current();
 Fl_Group::current(0);
 // create a new top level window
 Fl_Window *w = message_form = new Fl_Window(410,103, "提示");
  message_form->callback(button_cb_x);
 // w->clear_border();
 // w->box(FL_UP_BOX);
 (message = new Fl_Box(60, 25, 340, 20))
   ->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
 (input = new Fl_Input(60, 37, 340, 23))->hide();
 {Fl_Box* o = icon = new Fl_Box(10, 10, 50, 50);
  o->box(FL_THIN_UP_BOX);
  o->labelfont(FL_TIMES_BOLD);
  o->labelsize(34);
  o->color(FL_WHITE);
  o->labelcolor(FL_BLUE);
  //o->labelcolor(FL_RED);
 }
 w->end(); // don't add the buttons automatically
 // create the buttons (right to left)
 {
   for (int b=0, x=310; b<3; b++, x -= 100) {
     if (b==1)
       button[b] = new Fl_Return_Button(x, 70, 90, 23);
     else
       button[b] = new Fl_Button(x, 70, 90, 23);
     button[b]->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
     button[b]->callback(button_cb_x, b);
   }
 }
 button[0]->shortcut(FL_Escape);
 // add the buttons (left to right)
 {
   for (int b=2; b>=0; b--)
     w->add(button[b]);
 }
 w->begin();
 w->resizable(new Fl_Box(60,10,110-60,27));
 w->end();
 w->set_modal();
 Fl_Group::current(previously_current_group);
 return w;
}

/*
 * 'resizeform_x()' - Resize the form and widgets so that they hold everything
 *                  that is asked of them...
 */

static void resizeform_x() {
  int	i;
  int	message_w, message_h;
  int	text_height;
  int	button_w[3], button_h[3];
  int	x, w, h, max_w, max_h;
	const int icon_size = 50;

  message_form->size(410,103);

  fl_font(message->labelfont(), message->labelsize());
  message_w = message_h = 0;
  fl_measure(message->label(), message_w, message_h);

  message_w += 10;
  message_h += 10;
  if (message_w < 340)
    message_w = 340;
  if (message_h < 30)
    message_h = 30;

  button[0]->labelfont(FL_SCREEN_BOLD);

  fl_font(button[0]->labelfont(), button[0]->labelsize());

  memset(button_w, 0, sizeof(button_w));
  memset(button_h, 0, sizeof(button_h));

  for (max_h = 50, i = 0; i < 3; i ++)
    if (button[i]->visible())
    {
      fl_measure(button[i]->label(), button_w[i], button_h[i]);

      if (i == 1)
        button_w[1] += 20;

      button_w[i] += 64;
      button_h[i] += 10;

      if (button_h[i] > max_h)
        max_h = button_h[i];
    }

  if (input->visible()) text_height = message_h + 25;
  else text_height = message_h;

  max_w = message_w + 10 + icon_size;
  w     = button_w[0] + button_w[1] + button_w[2] - 10;

  if (w > max_w)
    max_w = w;

  message_w = max_w - 10 - icon_size;

  w = max_w + 20;
  h = max_h + 30 + text_height;

  message_form->size(w, h);
  message_form->size_range(w, h, w, h);

  message->resize(20 + icon_size, 10, message_w, message_h);
  icon->resize(10, 10, icon_size, icon_size);
  icon->labelsize(icon_size - 10);
  input->resize(20 + icon_size, 10 + message_h, message_w, 25);

  for (x = w, i = 0; i < 3; i ++)
    if (button_w[i])
    {
      x -= button_w[i];
      button[i]->resize(x, h - 10 - max_h, button_w[i] - 10, max_h);

//      printf("button %d (%s) is %dx%d+%d,%d\n", i, button[i]->label(),
//             button[i]->w(), button[i]->h(),
//	     button[i]->x(), button[i]->y());
    }
}

static int innards_x(const char* fmt, va_list ap,
  const char *b0,
  const char *b1,
  const char *b2)
{
  Fl::pushed(0); // stop dragging (STR #2159)

  avoidRecursion = 1;

  makeform_x();
  message_form->size(410,103);
  char buffer[1024];
  if (!strcmp(fmt,"%s")) {
    message->label(va_arg(ap, const char*));
  } else {
    ::vsnprintf(buffer, 1024, fmt, ap);
    message->label(buffer);
  }


  message_form->color(FL_YELLOW);
  message->labelcolor(FL_BLACK);
  //button[1]->color(FL_WHITE);
  //button[1]->box(FL_FLAT_BOX);
  //button[1]->down_box(FL_FLAT_BOX);
  button[1]->labelsize(20);
  button[1]->labelcolor(FL_BLACK);
	button[1]->labelfont(FL_BOLD);

  message->labelfont(s_fl_message_font_x);
  if (s_fl_message_size_x == -1)
    message->labelsize(FL_NORMAL_SIZE);
  else
    message->labelsize(s_fl_message_size_x);
  if (b0) {button[0]->show(); button[0]->label(b0); button[1]->position(210,70);}
  else {button[0]->hide(); button[1]->position(310,70);}
  if (b1) {button[1]->show(); button[1]->label(b1);}
  else button[1]->hide();
  if (b2) {button[2]->show(); button[2]->label(b2);}
  else button[2]->hide();
  const char* prev_icon_label = icon->label();
  if (!prev_icon_label) icon->label(iconlabel);

  resizeform_x();

  //if (button[1]->visible() && !input->visible())
  //  button[1]->take_focus();
  if (enableHotspot)
    message_form->hotspot(button[0]);
  if (b0 && Fl_Widget::label_shortcut(b0))
    button[0]->shortcut(0);
  else
    button[0]->shortcut(FL_Escape);

  // set default window title, if defined and a specific title is not set
  if (!message_form->label() && message_title_default)
    message_form->label(message_title_default);

  // deactivate Fl::grab(), because it is incompatible with modal windows
  Fl_Window* g = Fl::grab();
  if (g) Fl::grab(0);
  Fl_Group *current_group = Fl_Group::current(); // make sure the dialog does not interfere with any active group
  message_form->show();
  Fl_Group::current(current_group);
  while (message_form->shown()) Fl::wait();
  if (g) // regrab the previous popup menu, if there was one
    Fl::grab(g);
  icon->label(prev_icon_label);
  message_form->label(0); // reset window title

  avoidRecursion = 0;
  return ret_val;
}

void fl_message_title_x(const char *title) {
  makeform_x();
  message_form->copy_label(title);
}


void fl_message_pos_x(int x, int y)
{
    message_form->position(x, y);
    message_form->redraw();
}
/** Shows an information message dialog box.

   \note Common dialog boxes are application modal. No more than one common dialog box
   can be open at any time. Requests for additional dialog boxes are ignored.
   \note \#include <FL/fl_ask.H>


   \param[in] fmt can be used as an sprintf-like format and variables for the message text
 */
void fl_message_x(const char *fmt, ...) {

  if (avoidRecursion) return;

  va_list ap;

  // fl_beep_x(FL_BEEP_MESSAGE);

  va_start(ap, fmt);
  iconlabel = "i";
  innards_x(fmt, ap, 0, fl_close_x, 0);
  va_end(ap);
  iconlabel = "?";
}

void fl_message_font_x(Fl_Font f, Fl_Fontsize s)
{
   s_fl_message_font_x = f;
   s_fl_message_size_x = s;
}

//
// End of "$Id$".
//
