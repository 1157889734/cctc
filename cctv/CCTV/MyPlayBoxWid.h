#ifndef _MY_PLAY_BOX_WID_H_
#define _MY_PLAY_BOX_WID_H_
#include <FL/Fl_Box.H>

class FL_EXPORT My_Play_Box : public Fl_Box {

public:
  /**
    - The first constructor sets box() to FL_NO_BOX, which
    means it is invisible. However such widgets are useful as placeholders
    or Fl_Group::resizable()
    values.  To change the box to something visible, use box(n).
    - The second form of the constructor sets the box to the specified box
    type.
    <P>The destructor removes the box.
  */
  My_Play_Box(int X, int Y, int W, int H, const char *l=0);

  virtual int handle(int);
  void draw() ;
};

#endif