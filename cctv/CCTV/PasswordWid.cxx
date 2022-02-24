#include "PasswordWid.h"
#include "md5.h"
#include <string.h>  
#include <stdio.h>  
#include <FL/fl_ask.H>
#include "state.h"
#include "message_x.h"


static void PassBtnPressFun(Fl_Widget* pWid,void *pData)
{
	PasswordWid *pMyPassWid = (PasswordWid *)pData;
	Fl_Secret_Input *pInput = pMyPassWid->m_pInPass;
	if(pInput->size() >=8 && pWid !=  pMyPassWid->m_pBtnPass[10] 
		&& pWid !=  pMyPassWid->m_pBtnPass[11] && pWid !=  pMyPassWid->m_pBtnPass[12])
	{
		return ;
	}
	if(pWid == pMyPassWid->m_pBtnPass[0])
	{	
		pInput->insert("1");
	}
	else if(pWid == pMyPassWid->m_pBtnPass[1])
	{	
		pInput->insert("2");
	}else if(pWid == pMyPassWid->m_pBtnPass[2])
	{	
		pInput->insert("3");
	}else if(pWid == pMyPassWid->m_pBtnPass[3])
	{	
		pInput->insert("4");
	}else if(pWid == pMyPassWid->m_pBtnPass[4])
	{	
		pInput->insert("5");
	}else if(pWid == pMyPassWid->m_pBtnPass[5])
	{	
		pInput->insert("6");
	}else if(pWid == pMyPassWid->m_pBtnPass[6])
	{	
		pInput->insert("7");
	}else if(pWid == pMyPassWid->m_pBtnPass[7])
	{	
		pInput->insert("8");
	}else if(pWid == pMyPassWid->m_pBtnPass[8])
	{	
		pInput->insert("9");
	}else if(pWid == pMyPassWid->m_pBtnPass[9])
	{	
		pInput->insert("0");
	}else if(pWid == pMyPassWid->m_pBtnPass[10]) //DEL 按钮
	{	
		int iSize = pInput->size();
		if(iSize >0)
		{
			pInput->cut(iSize-1,iSize);
		}
	}else if(pWid == pMyPassWid->m_pBtnPass[11])  //确认按钮
	{
		const char *pcEnterPass = pInput->value(); 
		char acPwd[32] = {0};
		unsigned char acData[16] = {0};
		unsigned char acBuffer[16] = {0};
		FILE *file = NULL;  
		char acFileFullName[256] = {0};
		
		strncpy(acPwd,pcEnterPass,sizeof(acPwd));
		MD5_String(acPwd,acData);
		
		snprintf(acFileFullName,sizeof(acFileFullName)-1,PASSWORDDIR"/Password");
    	if ((file = fopen (acFileFullName, "rb")) == NULL) 
    	{
    		if(strcmp(acPwd,"12345"))
    		{
    			pMyPassWid->m_pBoxMessage->copy_label("密码错误");
				pMyPassWid->m_pBoxMessage->redraw();
				return ;
    		}
			if ((file = fopen(acFileFullName, "ab")) == NULL)
    		{
    			return ;
    		}
			fwrite(acData,sizeof(acData),1,file );
			fclose(file);
    	}
		else
		{
			fread(acBuffer, 1, sizeof(acBuffer),file);
			if(memcmp(acBuffer,acData,sizeof(acData)))
			{
				pMyPassWid->m_pBoxMessage->copy_label("密码错误");
				pMyPassWid->m_pBoxMessage->redraw();
				fclose(file);
				return ;
			}
			fclose(file);
		}
		if(pMyPassWid->m_pBoxTip->visible())
		{
			fl_close_x = "确定";
			fl_message_title_x("OK");
			fl_message_pos_x(210, 420);
			fl_message_font_x(FL_SCREEN_BOLD, 28);
			fl_message_x("特别注意:切换后请将故障显示屏电源关闭!\n\n");
		}
		
        pMyPassWid->m_iResult = 1;
		pInput->value("");
		pMyPassWid->m_pBoxMessage->copy_label("");
		pMyPassWid->m_pBoxMessage->redraw();
   		if(pMyPassWid->m_pHideCBFun)
		{
			pMyPassWid->m_pHideCBFun((Fl_Widget*)pData,pMyPassWid->m_pData);
		}
    }  
	else if(pWid == pMyPassWid->m_pBtnPass[12]) //取消按钮
	{
		pMyPassWid->m_iResult = 0;
		pInput->value("");
		pMyPassWid->m_pBoxMessage->copy_label("");
		pMyPassWid->m_pBoxMessage->redraw();
		if(pMyPassWid->m_pHideCBFun)
		{
			pMyPassWid->m_pHideCBFun((Fl_Widget*)pData,pMyPassWid->m_pData);
		}
	}
}


PasswordWid::PasswordWid(int x,int y,int w,int h,const char *l):
Fl_Double_Window(x,y,w,h,l)
{
	begin();
	clear_border();
	color((Fl_Color)0x30529200);
	m_pInPass = new Fl_Secret_Input(400,140,224,35,"请输入密码: ");
    m_pInPass->labelcolor(FL_WHITE);
	m_pInPass->labelfont(FL_BOLD);
	m_pInPass->maximum_size(6);
	m_pInPass->labelsize(18);
	m_iResult = 0;
	m_pData = 0;
	m_pHideCBFun = NULL;
	for(int i=0;i<13;i++)
    {  	
    	int iLeft = 282 + 120 *(i%4);
		int iTop =  200 +(i/4) *70;
		char acNO[8] ;
		memset(acNO,0,sizeof(acNO));
		sprintf(acNO, "%d",i+1);	
		switch(i)
		{
			case 9:
				m_pBtnPass[i] = new Fl_Button(iLeft, iTop, 100,55,"0");
				break;
			case 10:
				m_pBtnPass[i] = new Fl_Button(iLeft, iTop, 220,55,"Del");
				break;
			case 11:
				m_pBtnPass[i] = new Fl_Button(370, iTop+80, 120,60,"确定");
				break;
			case 12:
				m_pBtnPass[i] = new Fl_Button(534, iTop+10, 120,60,"取消");
				break;
			default:
				m_pBtnPass[i] = new Fl_Button(iLeft, iTop, 100,55);
				m_pBtnPass[i]->copy_label(acNO);
		}
						
		m_pBtnPass[i]->box(FL_FLAT_BOX);
      	m_pBtnPass[i]->down_box(FL_FLAT_BOX);
      	m_pBtnPass[i]->color(FL_WHITE);
		m_pBtnPass[i]->selection_color((Fl_Color)0xA0EEEE00);
      	m_pBtnPass[i]->labelsize(20);
      	m_pBtnPass[i]->labelcolor(FL_BLACK);
		m_pBtnPass[i]->labelfont(FL_BOLD);
		m_pBtnPass[i]->callback(PassBtnPressFun,this);
    }
	
	m_pBoxMessage = new Fl_Box(630,140,200,35,""); 
	m_pBoxMessage->labelcolor(FL_RED);
	m_pBoxMessage->labelsize(18);

    m_pBoxTiTle = new Fl_Box(300,40,424,35,""); 
	m_pBoxTiTle->labelcolor(FL_WHITE);
	m_pBoxTiTle->labelsize(20);
    m_pBoxTiTle->align(Fl_Align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE));
	
	m_pBoxTip = new Fl_Box(300,65,424,65,"特别注意:切换后请将故障显示屏电源关闭!"); 
	m_pBoxTip->labelcolor(FL_YELLOW);
	m_pBoxTip->labelsize(30);
    m_pBoxTip->align(Fl_Align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE));
	
	end();
}

PasswordWid::~PasswordWid()
{
	
}

int PasswordWid::GetResult()
{
	return m_iResult;
}

void PasswordWid::SetHideCBFun(HideCBFun *pFun,void *pData)
{
	m_pHideCBFun = pFun;
	m_pData = pData;
}

void PasswordWid::SetTiTle(char *pText)
{
    m_pBoxTiTle->copy_label(pText);
    m_pBoxTiTle->redraw();
}
void PasswordWid::SetBoxTipVisible(int iShow)
{
	if(iShow)
	{
		m_pBoxTip->show();
	}
	else
	{
		m_pBoxTip->hide();
	}
}


