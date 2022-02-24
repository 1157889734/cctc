#include "MoniManageWid.h"

void SysManBtnClicked(Fl_Widget* pWid,void *pData)
{
	MoniManageWid *pMoniWid = (MoniManageWid *)pData;
	pMoniWid->m_pSysManaWid->show();
	pMoniWid->m_pVidManaWid->hide();
	pMoniWid->m_pBtn1[0]->value(1);
	pMoniWid->m_pBtn1[1]->value(0);
}

void VideoManBtnClicked (Fl_Widget* pWid,void *pData)
{
	MoniManageWid *pMoniWid = (MoniManageWid *)pData;
	pMoniWid->m_pVidManaWid->show();
	pMoniWid->m_pSysManaWid->hide();
	pMoniWid->m_pBtn1[1]->value(1);
	pMoniWid->m_pBtn1[0]->value(0);
}

MoniManageWid::MoniManageWid(int x,int y,int w, int h,const char* l)
	:Fl_Double_Window(x,y,w,h,l)
{
   SetUi();
}




int MoniManageWid::SetUi()
{
	begin();
	clear_border();
	color((Fl_Color)0x30529200);
	for(int i=0;i<2;i++)
	{
		if(0 == i)
		{
			m_pBtn1[i] = new Fl_Button(0, 0, 512, 60, "系统管理");
		}
		else if(1 == i)
		{
			m_pBtn1[i] = new Fl_Button(512, 0, 512, 60, "录像管理");
		}
		
		m_pBtn1[i]->box(FL_BORDER_BOX);
      	m_pBtn1[i]->down_box(FL_DOWN_BOX);
      	m_pBtn1[i]->color((Fl_Color)181);
		m_pBtn1[i]->selection_color((Fl_Color)FL_YELLOW);
      	m_pBtn1[i]->labelsize(22);
      	m_pBtn1[i]->labelcolor(FL_BLACK);
	}
	m_pBtn1[0]->callback(SysManBtnClicked,this);
	m_pBtn1[1]->callback(VideoManBtnClicked,this);
	m_pBtn1[0]->value(1);
	m_pSysManaWid = new SysManageWid(0, 60, 1024, 708);
	m_pVidManaWid = new VideoManageWid(0,60,1024,708);
	end();
	m_pVidManaWid->hide();
	return 0;
}

static void CallHideFunTimer(void *arg)
{
    MoniManageWid *pMoniWid= (MoniManageWid*)arg;	
    
	pMoniWid->m_pBackCBFun(pMoniWid,pMoniWid->m_pData);
    
    Fl::remove_timeout(CallHideFunTimer,arg);
}


static void BackBtnClicked(Fl_Widget*,void *pData)
{
    MoniManageWid *pMoniWid = (MoniManageWid *)pData;

    //当存在播放时，如果关闭后直接掉切换界面的函数，
    //会存在堵塞，界面显示暂时异常的问题
    if(pMoniWid->m_pVidManaWid->closePlayVideo())
    {
        Fl::add_timeout(0.001, CallHideFunTimer, pData); 
    }else
    {
        pMoniWid->m_pBackCBFun(pMoniWid,pMoniWid->m_pData);
    }

    
}

void MoniManageWid::SetBackCBFun(BackCBFun *pBackFun,void *pData)
{
    m_pBackCBFun = pBackFun;
    m_pData = pData;
    m_pSysManaWid->m_pBtnOper[6]->callback(BackBtnClicked, this);
    m_pVidManaWid->m_pBtnOper[9]->callback(BackBtnClicked, this);
}   

