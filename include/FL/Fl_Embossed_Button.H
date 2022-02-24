#ifndef _FL_MYBUTTON_H_
#define _FL_MYBUTTON_H_
#include<FL/Fl_Button.H>
class FL_EXPORT Fl_Embossed_Button:public Fl_Button{
private:
  int code_;

protected:
  virtual void  draw();

public:

Fl_Embossed_Button(int x,int y,int w,int h,const char*l);
// virtual  int hand(int);
 
 int getcode(){return code_;}
 void setcode(int c){code_ = c;}
};



#endif
