#include <FL/fl_ask.H>
#include "AVManageWid.h"
#include <string.h>  
#include <stdio.h>  
#include "pmsgproc.h"
#include "state.h"
#include "log.h"

void CheckSourceTimer(void*arg)
{
	AVManageWid *pAVManaWid = (AVManageWid *)arg;
	int iMode = GetVideoSource();
	if(pAVManaWid->m_iSrcMode != iMode)
	{
		pAVManaWid->m_iSrcMode = iMode;
		if(0x11 == iMode)
		{
			pAVManaWid->m_pRdBtn[0]->value(1);
			pAVManaWid->m_pRdBtn[1]->value(0);
		}
		else
		{
			pAVManaWid->m_pRdBtn[0]->value(0);
			pAVManaWid->m_pRdBtn[1]->value(1);
		}
	}
}

static void ReturnBtnClick(Fl_Widget* pWid,void *pData)
{	
	AVManageWid *pAVManaWid = (AVManageWid *)pData;
	Fl_Double_Window *pCCTVWid = (Fl_Double_Window *)pAVManaWid->m_pData;
	
	pAVManaWid->m_pBoxMessage->copy_label("");
	pCCTVWid->show();
	Fl::remove_timeout(CheckSourceTimer,pData);
	pAVManaWid->hide();
}

static void SetBtnClick(Fl_Widget* pBtn,void *pData)
{
	AVManageWid *pAVManaWid = (AVManageWid *)pData;
	int iValue = 0;

	if(pBtn == pAVManaWid->m_pBtn[0])
	{
		int iRet = 0; 
		int iSrc = (1 == GetDeviceCarriageNo())?1:2;			
		
		if(pAVManaWid->m_pRdBtn[0]->value())
		{
			iRet = SwitchVideoSrc(iSrc ,0x11);
			iValue = 0x11;
		}
		else
		{
			iRet = SwitchVideoSrc(iSrc ,0x13);
			iValue = 0x13;
		}
		if(iRet <=0)
		{
			if(0x11 == pAVManaWid->m_iSrcMode)
			{
				pAVManaWid->m_pRdBtn[1]->value(0);
				pAVManaWid->m_pRdBtn[0]->value(1);
				pAVManaWid->m_iSrcMode = 0x11;
			}
			else
			{
				pAVManaWid->m_pRdBtn[0]->value(0);
				pAVManaWid->m_pRdBtn[1]->value(1);
			}
            pAVManaWid->m_pBoxMessage->copy_label("设置视频源失败");
            pAVManaWid->m_pBoxMessage->redraw();
		}
		else
		{
			T_LOG_INFO tLog;

			memset(&tLog,0,sizeof(T_LOG_INFO));
			tLog.iLogType = LOG_TYPE_EVENT;	
			if(pAVManaWid->m_pRdBtn[0]->value())
			{
				snprintf(tLog.acLogDesc,sizeof(tLog.acLogDesc)-1
				,"set video source to online play");
			}
			else
			{
				snprintf(tLog.acLogDesc,sizeof(tLog.acLogDesc)-1
				,"set video source to local broadcast");
			}
			
			LOG_WriteLog(&tLog);
			pAVManaWid->m_pBoxMessage->copy_label("设置视频源成功");
            pAVManaWid->m_pBoxMessage->redraw();
			Fl::remove_timeout(CheckSourceTimer,pData);
			Fl::add_timeout(3,CheckSourceTimer,pData);	
		}
	}		
	else if(pBtn == pAVManaWid->m_pBtn[1])
	{
		iValue = (int )pAVManaWid->m_pMySlider[0]->value();
		SetLcdVolumeValue(iValue);
	}
	else if(pBtn == pAVManaWid->m_pBtn[2] || 
        pBtn == pAVManaWid->m_pBtn[3])
	{
	    T_CARRIAGE_SPEAKER_VOL tSpeakerVol ;
        
        tSpeakerVol.value_train = (char)pAVManaWid->m_pMySlider[2]->value();
        for(int i=0;i<6;i++)
        {
            tSpeakerVol.value[i] = tSpeakerVol.value_train;
        }
        //如果音量不是是自动调节
        //当司机室音量为0时，静音
        if(pAVManaWid->m_pCheckBtn[0]->value())
        {
            tSpeakerVol.ascu_mute[0] = 0;
            tSpeakerVol.ascu_mute[1] = 0;

            tSpeakerVol.value_monitor[0] = 0xff;
            tSpeakerVol.value_monitor[1] = 0xff;
        }
        else
        {
            tSpeakerVol.value_monitor[0] = (char)pAVManaWid->m_pMySlider[1]->value();
            tSpeakerVol.value_monitor[1] = tSpeakerVol.value_monitor[0];
            
            if (0 == tSpeakerVol.value_monitor[0])
            {
                tSpeakerVol.ascu_mute[0] = 1;
                tSpeakerVol.ascu_mute[1] = 1;
            }
            else
            {
                tSpeakerVol.ascu_mute[0] = 0;
                tSpeakerVol.ascu_mute[1] = 0;
            }
        }     
        SetDriverRoomSpeakerVolume(pAVManaWid->m_pMySlider[1]->value());
        SetCarriageSpeakerVolume(pAVManaWid->m_pMySlider[2]->value());
        adjustCarriageSpeakerVolume(&tSpeakerVol);
	}
}

static void MoniCheckClicked(Fl_Widget* pBtn,void *pData)
{
	AVManageWid *pAVManaWid = (AVManageWid *)pData;
    
	if(pAVManaWid->m_pCheckBtn[0]->value())
	{
	    T_CARRIAGE_SPEAKER_VOL tSpeakerVol ;
        
		pAVManaWid->m_pBoxVol[0]->show();;
		pAVManaWid->m_pMySlider[1]->hide();
		pAVManaWid->m_pBtn[2]->hide();
		
        tSpeakerVol.value_train = (char)pAVManaWid->m_pMySlider[2]->value();
        for(int i=0;i<6;i++)
        {
            tSpeakerVol.value[i] = tSpeakerVol.value_train;
        }
 
        tSpeakerVol.ascu_mute[0] = 0;
        tSpeakerVol.ascu_mute[1] = 0;
        tSpeakerVol.value_monitor[0] = 0xff;
        tSpeakerVol.value_monitor[1] = 0xff;
        adjustCarriageSpeakerVolume(&tSpeakerVol);
	}
	else
	{
		pAVManaWid->m_pBoxVol[0]->hide();
		pAVManaWid->m_pMySlider[1]->show();
		pAVManaWid->m_pBtn[2]->show();
	}
}

static void NoiseCheckClicked(Fl_Widget* pBtn,void *pData)
{
	AVManageWid *pAVManaWid = (AVManageWid *)pData;
	if(pAVManaWid->m_pCheckBtn[1]->value())
	{
		pAVManaWid->m_pBoxVol[1]->show();
		pAVManaWid->m_pMySlider[2]->hide();
		pAVManaWid->m_pBtn[3]->hide();
		StartNoiseMonitor(0);
	}
	else
	{
		pAVManaWid->m_pBoxVol[1]->hide();
		pAVManaWid->m_pMySlider[2]->show();
		pAVManaWid->m_pBtn[3]->show();
		StartNoiseMonitor(1);
	}
}

AVManageWid::AVManageWid(int x,int y,int w, int h,const char* l)
	:Fl_Double_Window(x,y,w,h,l)
{
   SetUi();
   m_pData = NULL;  
}

AVManageWid::~AVManageWid()
{
	
}

int AVManageWid::SetUi()
{
	/*char acRunPath[256] ={0};
	char acImgPath[256] ={0};

    m_pBoxBg = new Fl_Box(0,0,1024,768);
    GetCCTVRunDir(acRunPath, sizeof(acRunPath));
    snprintf(acImgPath,sizeof(acImgPath)-1,"%s/res/bg2.png",acRunPath);
    m_pBgImg = new Fl_PNG_Image(acImgPath);
    m_pBoxBg->image(m_pBgImg);*/
    
	begin();
	clear_border();
	m_i8VolMoni = 10;
	m_i8VolTrain = 10;
    color((Fl_Color)0x30529200);

    m_pBoxMessage = new Fl_Box(242,30,540,40,""); 
	m_pBoxMessage->labelcolor(FL_RED);
	m_pBoxMessage->labelsize(18);
    
    Fl_Group *pGroup1 = new Fl_Group(124,71,786,285,"  视频源切换");
    pGroup1->box(FL_BORDER_FRAME);
    pGroup1->color(FL_WHITE);
    pGroup1->labelsize(22);
    pGroup1->labelcolor(FL_WHITE);
    pGroup1->align(Fl_Align(37|FL_ALIGN_INSIDE));
    
	m_pRdGroup = new Fl_Group(410,100,204,245,"视频源");
    m_pRdGroup->box(FL_BORDER_FRAME);
    m_pRdGroup->color(FL_WHITE);
    m_pRdGroup->labelsize(22);
    m_pRdGroup->labelcolor(FL_WHITE);
    m_pRdGroup->align(Fl_Align(33|FL_ALIGN_INSIDE));
	m_pRdBtn[0]= new Fl_Round_Button(445, 180, 120, 30, " 直播");  //直播
	m_pRdBtn[1] = new Fl_Round_Button(445, 240, 120, 30, " 录播"); //录播

	for(int i=0;i<2;i++)
	{
		m_pRdBtn[i]->type(102);
      	m_pRdBtn[i]->down_box(_FL_ROUND_DOWN_BOX);
      	m_pRdBtn[i]->color((Fl_Color)181);
		m_pRdBtn[i]->selection_color((Fl_Color)FL_BLACK);
      	m_pRdBtn[i]->labelsize(20);
      	m_pRdBtn[i]->labelcolor(FL_WHITE);
	}
	m_pRdBtn[1]->value(1);
	m_pRdGroup->end();
    m_pBtn[0] = new  Fl_Button (760,240,100,40,"确定");
    pGroup1->end();

    Fl_Group *pGroup2 = new Fl_Group(124,400,786,303,"  音量控制");
    pGroup2->box(FL_BORDER_FRAME);
    pGroup2->color(FL_WHITE);
    pGroup2->labelsize(22);
    pGroup2->labelcolor(FL_WHITE);
    pGroup2->align(Fl_Align(37|FL_ALIGN_INSIDE));
	
	m_pCheckBtn[0]= new Fl_Check_Button(445, 410, 120, 30, " 监听开关");  //监听开关
	m_pCheckBtn[1] = new Fl_Check_Button(600, 410, 120, 30, " 噪检开关"); //噪检开关
	m_pCheckBtn[0]->callback(MoniCheckClicked, this);
	m_pCheckBtn[1]->callback(NoiseCheckClicked, this);
	for(int i=0;i<2;i++)
	{
		m_pCheckBtn[i]->type(FL_TOGGLE_BUTTON);
      	m_pCheckBtn[i]->down_box(FL_DOWN_BOX);
      	m_pCheckBtn[i]->color((Fl_Color)181);
		m_pCheckBtn[i]->selection_color((Fl_Color)FL_BLACK);
      	m_pCheckBtn[i]->labelsize(18);
      	m_pCheckBtn[i]->labelcolor(FL_WHITE);
	}

    Fl_Box *pLable1 = new Fl_Box(180,458,540,35,"客室LCD音量");
    pLable1->labelcolor(FL_WHITE);
    pLable1->labelsize(20);
    pLable1->labelfont(FL_BOLD);
    pLable1->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));

    Fl_Box *pLable2 = new Fl_Box(180,528,540,35,"司机室监听扬声器音量");
    pLable2->labelcolor(FL_WHITE);
    pLable2->labelsize(20);
    pLable2->labelfont(FL_BOLD);
    pLable2->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));

    Fl_Box *pLable3 = new Fl_Box(180,598,540,35,"客室广播音量");
    pLable3->labelcolor(FL_WHITE);
    pLable3->labelsize(20);
    pLable3->labelfont(FL_BOLD);
    pLable3->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	
	m_pBtn[1] = new  Fl_Button (760,490,100,40,"确定");
	m_pBtn[2] = new  Fl_Button (760,560,100,40,"确定");
	m_pBtn[3] = new  Fl_Button (760,630,100,40,"确定"); 
    m_pMySlider[0] = new MySlider(180,498,540,25);
	m_pMySlider[1] = new MySlider(180,568,540,25);
	m_pMySlider[2] = new MySlider(180,638,540,25);
	m_pBoxVol[0] = new Fl_Box(180,568,540,25,"音量不可调节"); //音量不可调节
	m_pBoxVol[1] = new Fl_Box(180,638,540,25,"音量不可调节"); //音量不可调节
    pGroup2->end();

    m_pBtn[4] = new  Fl_Button(900,715,100,40,"返回");
    
	
	for(int i =0;i<5;i++)
	{
		m_pBtn[i]->box(FL_UP_BOX);
      	m_pBtn[i]->down_box(FL_DOWN_BOX);
      	m_pBtn[i]->color((Fl_Color)181);
		m_pBtn[i]->selection_color((Fl_Color)6);
      	m_pBtn[i]->labelsize(18);
      	//m_pBtn[i]->labelcolor(FL_WHITE);
		m_pBtn[i]->labelfont(FL_BOLD);
	}
	m_pBtn[0]->callback(SetBtnClick, this);
	m_pBtn[1]->callback(SetBtnClick, this);
	m_pBtn[2]->callback(SetBtnClick, this);
	m_pBtn[3]->callback(SetBtnClick, this);
    m_pBtn[4]->callback(ReturnBtnClick, this);
	
	for(int i =0;i<2;i++)
	{
		m_pBoxVol[i]->labelcolor(FL_WHITE);
		m_pBoxVol[i]->labelsize(18);
		m_pBoxVol[i]->labelfont(FL_BOLD);
		m_pBoxVol[i]->hide();
	}
	for(int i=0;i<3;i++)
	{
		m_pMySlider[i]->type(FL_HOR_NICE_SLIDER);
    	m_pMySlider[i]->box(FL_FLAT_BOX);
    	m_pMySlider[i]->color((Fl_Color)0x30529200);
    	m_pMySlider[i]->step(1);
		m_pMySlider[i]->textcolor(FL_WHITE);
		m_pMySlider[i]->textsize(16);
		m_pMySlider[i]->textfont(FL_BOLD);
		m_pMySlider[i]->selection_color((Fl_Color)181);
	}
	m_pMySlider[0]->range(0,100);
	m_pMySlider[1]->range(0,5);
	m_pMySlider[2]->range(0,10);
	
	m_pMySlider[0]->value(GetLcdVolumeValue());
	m_pMySlider[1]->value(GetDriverRoomSpeakerVolume());
	m_pMySlider[2]->value(GetCarriageSpeakerVolume());

	//if(!GetDriverRoomMonitorFlag())
	{
		m_pCheckBtn[0]->value(1);
		m_pBoxVol[0]->show();
		m_pMySlider[1]->hide();
		m_pBtn[2]->hide();
	}
	
	//if(!GetNoiseMonitorFlag())
	{
		m_pCheckBtn[1]->value(0);
		m_pBoxVol[1]->hide();
		m_pMySlider[2]->show();
		m_pBtn[3]->show();
	}
	end();
	return 0;
}

void AVManageWid::SetDataPtr(void *pData)
{
	m_pData = pData;
}

void AVManageWid::UpdateState()
{
	int iMode = GetVideoSource();
	if(0x11 == iMode)
	{
		m_pRdBtn[0]->value(1);
		m_pRdBtn[1]->value(0);
		
		m_iSrcMode = 0x11;
	}
	else
	{
		m_pRdBtn[0]->value(0);
		m_pRdBtn[1]->value(1);
		m_iSrcMode = 0x13;
	}
	m_pMySlider[0]->value(GetLcdVolumeValue());
	m_pMySlider[1]->value(GetDriverRoomSpeakerVolume());
	m_pMySlider[2]->value(GetCarriageSpeakerVolume());
}

