#include "DateChoiceWid.h"
#include <string.h>  
#include <stdio.h>  
#include "comm.h"

static void BtnPressFun(Fl_Widget* pWid,void *pData)
{
	DateChoiceWid *pMyDateWid = (DateChoiceWid *)pData;
	int iYear = pMyDateWid->GetYear();
	int iMonth = pMyDateWid->GetMonth();
	int iDay = pMyDateWid->GetDay();
	int iMaxDay = 28;
	if(pWid == pMyDateWid->m_pBtnTime[0])
	{	
		iYear++;
		if(iYear >2100)
		{
			return ;
		}
		pMyDateWid->SetYear(iYear);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMyDateWid->SetDay(iMaxDay);
		}
	}
	else if(pWid == pMyDateWid->m_pBtnTime[1])
	{
		iYear--;
		if(iYear <1970)
		{
			return ;
		}	
		pMyDateWid->SetYear(iYear);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMyDateWid->SetDay(iMaxDay);
		}
	}else if(pWid == pMyDateWid->m_pBtnTime[2])
	{
		iMonth ++;
		if(iMonth >12)
		{
			iMonth = 1;
		}
		pMyDateWid->SetMonth(iMonth);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMyDateWid->SetDay(iMaxDay);
		}
	}else if(pWid == pMyDateWid->m_pBtnTime[3])
	{
		iMonth--;
		if(iMonth <1)
		{
			iMonth = 12;
		}
		pMyDateWid->SetMonth(iMonth);
		iMaxDay = GetMaxDay(iYear,iMonth);
		if(iDay > iMaxDay)
		{
			pMyDateWid->SetDay(iMaxDay);
		}
	}else if(pWid == pMyDateWid->m_pBtnTime[4] || pWid == pMyDateWid->m_pBtnTime[5])
	{
		iMaxDay =GetMaxDay(iYear,iMonth);
		if(pWid == pMyDateWid->m_pBtnTime[4])
		{
			iDay ++;
		}
		else
		{
			iDay --;
		}
		if(iDay >iMaxDay)
		{
			iDay = 1;
		}
		if(iDay <1)
		{
			iDay = iMaxDay;
		}
		pMyDateWid->SetDay(iDay);
	}else if(pWid == pMyDateWid->m_pBtnTime[6])
	{
		int iHour = pMyDateWid->GetHour();
		iHour++;
		if(iHour >23)
		{
			iHour =0;
		}
		pMyDateWid->SetHour(iHour);
	}
	else if(pWid == pMyDateWid->m_pBtnTime[7])
	{
		int iHour = pMyDateWid->GetHour();
		iHour--;
		if(iHour <0)
		{
			iHour =23;
		}
		pMyDateWid->SetHour(iHour);
	}
	else if(pWid == pMyDateWid->m_pBtnTime[8])
	{
		int iMin = pMyDateWid->GetMin();
		iMin++;
		if(iMin >59)
		{
			iMin =0;
		}
		pMyDateWid->SetMin(iMin);
	}
	else if(pWid == pMyDateWid->m_pBtnTime[9])
	{
		int iMin = pMyDateWid->GetMin();
		iMin--;
		if(iMin <0)
		{
			iMin =59;
		}
		pMyDateWid->SetMin(iMin);
	}
	else if(pWid == pMyDateWid->m_pBtnTime[10])
	{
		int iSec = pMyDateWid->GetSec();
		iSec++;
		if(iSec >59)
		{
			iSec =0;
		}
		pMyDateWid->SetSec(iSec);
	}
	else if(pWid == pMyDateWid->m_pBtnTime[11])
	{
		int iSec = pMyDateWid->GetSec();
		iSec--;
		if(iSec <0)
		{
			iSec =59;
		}
		pMyDateWid->SetSec(iSec);
	}
	else if(pWid == pMyDateWid->m_pBtnOper[0])
	{
		pMyDateWid->m_iResult =1;
		pMyDateWid->hide();
	}else if(pWid == pMyDateWid->m_pBtnOper[1])
	{
		pMyDateWid->m_iResult =0;
		pMyDateWid->hide();
	}
}


DateChoiceWid::DateChoiceWid(int x,int y,int w,int h,const char *l):
Fl_Double_Window(x,y,w,h,l)
{
	begin();
	m_iYear  = 2018;
	m_iMonth = 9;
	m_iDay	 = 5;
	m_iHour  = 0;
	m_iMin	 = 0;
	m_iSec   = 0;
	char acTime[12] = {0};
	m_pDateCBFun = NULL;

	for(int i=0;i<2;i++)
	{
		if(0 == i)
		{
			m_pBoxTitle[i] = new Fl_Box(10,14,340,38,"日期-----------------------------------------");
		}
		else if(1 == i)
		{
			m_pBoxTitle[i] = new Fl_Box(10,190,300,38,"时间-----------------------------------------");
		}
		m_pBoxTitle[i]->labelcolor(FL_WHITE);
		m_pBoxTitle[i]->labelsize(20);
		m_pBoxTitle[i]->labelfont(FL_BOLD);
		m_pBoxTitle[i]->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	}
	for(int j=0;j<2;j++)
	{
		for(int i=0;i<3;i++)
		{
			m_pBtnTime[i*2+j*6 ] = new Fl_Button(10 +i* 110, 60 +j*170, 90, 40,"+");
			m_pBoxTime[i+j*3   ] = new Fl_Box(10 +i*110,100 +j*170,90,40);
			m_pBtnTime[i*2+j*6+1] = new Fl_Button(10 +i*110, 140+j*170, 90, 40,"-");

			for(int k= i*2+j*6;k<i*2+j*6 +2;k++)
			{
				m_pBtnTime[k]->box(FL_FLAT_BOX);
				m_pBtnTime[k]->down_box(FL_FLAT_BOX);
      			m_pBtnTime[k]->color((Fl_Color)FL_WHITE);
				m_pBtnTime[k]->selection_color((Fl_Color)0xBBBBBB00);
      			m_pBtnTime[k]->labelsize(22);
				m_pBtnTime[k]->labelfont(FL_BOLD);
				m_pBtnTime[k]->callback(BtnPressFun,this);
			}
			//m_pBoxTime[i+j*3]->labelcolor(FL_WHITE);
			m_pBoxTime[i+j*3]->labelsize(20);
			m_pBoxTime[i+j*3]->box(FL_FLAT_BOX);
			m_pBoxTime[i+j*3]->color(FL_GRAY);
		}
	}
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%d",m_iYear);
	m_pBoxTime[0]->copy_label(acTime);
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iMonth);
	m_pBoxTime[1]->copy_label(acTime);
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iDay);
	m_pBoxTime[2]->copy_label(acTime);
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iHour);
	m_pBoxTime[3]->copy_label(acTime);
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iMin);
	m_pBoxTime[4]->copy_label(acTime);
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime,"%02d",m_iSec);
	m_pBoxTime[5]->copy_label(acTime);
	
	for(int i =0;i<2;i++)
	{
		if(0 == i)
		{
			m_pBtnOper[i] = new Fl_Button(50, 370, 100, 38,"确定");
		}
		else if(1 ==i)
		{
			m_pBtnOper[i] = new Fl_Button(190, 370, 100, 38,"取消");
		}
		m_pBtnOper[i]->box(FL_UP_BOX);
      	m_pBtnOper[i]->down_box(FL_DOWN_BOX);
      	m_pBtnOper[i]->color((Fl_Color)FL_GRAY);
		m_pBtnOper[i]->selection_color((Fl_Color)0xBBBBBB00);
      	m_pBtnOper[i]->labelsize(16);
      	//m_pBtnOper[i]->labelcolor(FL_WHITE);
		m_pBtnOper[i]->labelfont(FL_BOLD);
		m_pBtnOper[i]->callback(BtnPressFun,this);
	}
	end();
}

DateChoiceWid::~DateChoiceWid()
{
	
}

int DateChoiceWid::GetResult()
{
	return m_iResult;
}

int DateChoiceWid::handle(int ev)
{
 	if(ev == FL_HIDE)
 	{
 		if(m_pDateCBFun)
		{
			m_pDateCBFun(this,m_pData);
		}
 	}
	return Fl_Double_Window::handle(ev);
}

void DateChoiceWid::SetHideCBFun(DateCBFun *pFun)
{
	m_pDateCBFun = pFun;
}

void DateChoiceWid::SetDataPtr(void *pData)
{
	m_pData = pData;
}

void DateChoiceWid::GetTimeStr(char *pStrDate)
{
	sprintf(pStrDate,"%d-%02d-%02d %02d:%02d:%02d",m_iYear,m_iMonth,m_iDay,m_iHour,m_iMin,m_iSec);
}

int  DateChoiceWid::GetYear()
{
	return m_iYear;
}
	
int  DateChoiceWid::GetMonth()
{
	return m_iMonth;
}
	
int  DateChoiceWid::GetDay()
{
	return m_iDay;
}

void  DateChoiceWid::SetYear( int iYear)
{
	if(m_iYear != iYear)
	{
		char acData[12] = {0};
		sprintf(acData,"%d",iYear);
		m_iYear = iYear;
		m_pBoxTime[0]->copy_label(acData);
		m_pBoxTime[0]->redraw();
	}
}
	
void  DateChoiceWid::SetMonth(int iMonth)
{
	if(m_iMonth != iMonth)
	{
		char acData[12] = {0};
		sprintf(acData,"%02d",iMonth);
		m_iMonth = iMonth;
		m_pBoxTime[1]->copy_label(acData);
		m_pBoxTime[1]->redraw();
	}
}
	
void  DateChoiceWid::SetDay(int iDay)
{
	if(m_iDay != iDay)
	{
		char acData[12] = {0};
		sprintf(acData,"%02d",iDay);
		m_iDay = iDay;
		m_pBoxTime[2]->copy_label(acData);
		m_pBoxTime[2]->redraw();
	}
}

int DateChoiceWid::GetHour()
{
	return m_iHour;
}
int  DateChoiceWid::GetMin()
{
	return m_iMin;
}
int  DateChoiceWid::GetSec()
{
	return m_iSec;
}

void  DateChoiceWid::SetHour(int iHour)
{
	if(m_iHour != iHour)
	{
		char acData[12] = {0};
		sprintf(acData,"%02d",iHour);
		m_iHour = iHour;
		m_pBoxTime[3]->copy_label(acData);
		m_pBoxTime[3]->redraw();
	}
}
void  DateChoiceWid::SetMin(int iMin)
{
	if(m_iMin != iMin)
	{
		char acData[12] = {0};
		sprintf(acData,"%02d",iMin);
		m_iMin = iMin;
		m_pBoxTime[4]->copy_label(acData);
		m_pBoxTime[4]->redraw();
	}
}

void  DateChoiceWid::SetSec(int iSec)
{
	if(m_iSec != iSec)
	{
		char acData[12] = {0};
		sprintf(acData,"%02d",iSec);
		m_iSec = iSec;
		m_pBoxTime[5]->copy_label(acData);
		m_pBoxTime[5]->redraw();
	}
}








