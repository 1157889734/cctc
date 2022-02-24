//
// exercisetablerow -- Exercise all aspects of the Fl_Table_Row widget
//

#include "MyTable.h"
#include <FL/Fl_Check_Button.H>
#include "state.h"
#include "NVRMsgProc.h"


MyTable::MyTable(int x, int y, int w, int h, const char *l) : Fl_Table_Row(x,y,w,h,l)
{
    begin();

    m_cell_bgcolor = FL_WHITE;
    m_cell_fgcolor = FL_BLACK;
	m_eTableType = E_LOG_TABLE;
	m_Playcolor = FL_BLUE;
	m_iPlayIndex = -1;
	m_pLogInfo = NULL;
	m_iLogNum = 0;
	m_iLogPage = 0;
	m_iLogType = 0;
    callback(&event_callback, (void*)this);
	end();
}

MyTable::~MyTable()
{
	if(m_pLogInfo)
	{
		free(m_pLogInfo);
		m_pLogInfo = NULL;
	}
}




Fl_Color MyTable::GetCellFGColor() 
{ 
	return(m_cell_fgcolor); 
}
Fl_Color MyTable::GetCellBGColor() 
{ 
	return(m_cell_bgcolor); 
}
void MyTable::SetCellFGColor(Fl_Color val) 
{ 
	m_cell_fgcolor = val; 
}
void MyTable::SetCellBGColor(Fl_Color val)
{ 
	m_cell_bgcolor = val; 
}



// Handle drawing all cells in table
void MyTable::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
    char s[256] = {0};
    switch ( context )
    {
	case CONTEXT_STARTPAGE:
	    fl_font(FL_HELVETICA, 16);
	    return;
	case CONTEXT_RC_RESIZE: 
	{
      int X, Y, W, H;
      int index = 0;
      for ( int r = 0; r<rows(); r++ ) 
	  {
	  	if ( index >= children() ) 
			break;
	  	find_cell(CONTEXT_TABLE, r, 0, X, Y, W, H);
	  	child(index++)->resize(X+10,Y,20,H);
      }
      init_sizes();			// tell group children resized
      return;
    }

	case CONTEXT_COL_HEADER:
		if(E_LOG_TABLE == m_eTableType)
		{
			switch(C)
			{
				case 0:
					sprintf(s, "序号");	//序号
					break;
				case 1:
					sprintf(s, "类型");	//类型
					break;
				case 2:
					sprintf(s, "时间");  //时间
					break;
				case 3:
					sprintf(s, "日志信息"); 
					break;
				default:
					break;
			}
		}else if(m_eTableType == E_DISKST_TABLE)
		{
			switch (C)
			{
				
				case 0:
					sprintf(s, "设备位置");	  //设备位置
					break;
				case 1:
					sprintf(s, "硬盘个数");	 //硬盘个数
					break;
				case 2:
					sprintf(s, "硬盘容量");	//硬盘容量
					break;
				case 3:
					sprintf(s, "使用量");      //使用量
					break;
				case 4:
					sprintf(s, "硬盘状态");	//硬盘状态
					break;
				case 5:
					sprintf(s, "在线状态");	//设备状态
					break;
				default:
					break;
			}
		}else if(m_eTableType == E_VLIST_TABLE)
		{
			switch (C)
			{
				case 0:
					sprintf(s, "选中");	//选中
					break;
				case 1:
					sprintf(s, "序号");	//序号
					break;
				case 2:
					sprintf(s, "文件名");	//文件名
					break;
				default:
					break;
			}
		}
		
	    fl_push_clip(X, Y, W, H);
	    {
			fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, col_header_color());
			fl_color(FL_BLACK);
			fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
	    }
	    fl_pop_clip();
	    return;

	case CONTEXT_ROW_HEADER:
		
	    fl_push_clip(X, Y, W, H);
	    {
			fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, row_header_color());
			fl_color(FL_BLACK);
			fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
	    }
	    fl_pop_clip();
	    return;

	case CONTEXT_CELL:
	{	
		if(m_eTableType == E_VLIST_TABLE && 0 == C)
		{
			fl_push_clip(X, Y, 12, H);
			{
				fl_color( row_selected(R) ? selection_color() : m_cell_bgcolor);
				if(R == m_iPlayIndex)
				{
					fl_color(m_Playcolor);
				}
				fl_rectf(X, Y, 12, H);
							
				// BORDER
				fl_color(color()); 
				fl_rect(X, Y, 12, H);
	    	}
	    	fl_pop_clip();

			fl_push_clip(X+26, Y, W-26, H);
			{
				fl_color( row_selected(R) ? selection_color() : m_cell_bgcolor);
				if(R == m_iPlayIndex)
				{
					fl_color(m_Playcolor);
				}
				fl_rectf(X+26, Y, W-26, H);
							
				// BORDER
				fl_color(color()); 
				fl_rect(X+26, Y, W-26, H);
	    	}
	    	fl_pop_clip();
			return ;
		}
		fl_push_clip(X, Y, W, H);
		{
	        // BG COLOR
			fl_color( row_selected(R) ? selection_color() : m_cell_bgcolor);
			if(R == m_iPlayIndex)
			{
				fl_color(m_Playcolor);
			}
			fl_rectf(X, Y, W, H);
			// TEXT
			fl_color(m_cell_fgcolor);
			if(E_LOG_TABLE == m_eTableType)
			{
				switch(C)
				{
					case 0:
						sprintf(s, "%d",R+1 + (m_iLogPage)*10);	///序号
						break;
					case 1:
						if(0 == m_iLogType)
						{
							sprintf(s, "System");	//类型
						}else
						{
							sprintf(s, "Operator");	//类型
						}
						break;
					case 2:
						sprintf(s, "%4d-%02d-%02d %02d:%02d:%02d",m_pLogInfo[m_iLogPage*10 +R].tLogTime.year,
							m_pLogInfo[m_iLogPage*10 +R].tLogTime.month,
							m_pLogInfo[m_iLogPage*10 +R].tLogTime.day,
							m_pLogInfo[m_iLogPage*10 +R].tLogTime.hour,
							m_pLogInfo[m_iLogPage*10 +R].tLogTime.minute,
							m_pLogInfo[m_iLogPage*10 +R].tLogTime.second);	 //日期
						break;
					case 3:
						 sprintf(s, m_pLogInfo[m_iLogPage*10 +R].acLog);  //日志类型
						break;
					default:
						break;
				}
			}else if(m_eTableType == E_DISKST_TABLE)
			{
				int iData =0;
				switch (C)
				{
				case 0:
					sprintf(s, "%d车NVR",R+1);	//设备位置
					break;
				case 1:
					iData = GetNvrDiskNum(R);
					sprintf(s, "%d个",iData);	//硬盘个数
					break;
				case 2:
					iData = GetNvrDiskTotalSize(R);
					sprintf(s, "%dG",iData);	//磁盘容量
					break;
				case 3:
					iData = GetNvrDiskUsedSize(R);
					sprintf(s, "%dG",iData); 	//磁盘使用量
					break;
				case 4:
					iData = GetNvrDiskWarnState(R);
					if(E_SERV_STATUS_CONNECT == NVR_GetConnectStatus(R))
					{
						if(0 == iData)
						{
							sprintf(s, "正常");
						}
						else if(1 == iData)
						{
						sprintf(s, "丢失");
						}
						else if(2 == iData)
						{
							sprintf(s, "容量不足");
						}
					}
					
					break;
				case 5:                                     //设备状态		
					fl_color(FL_READ);
					sprintf(s, "离线");
					if(E_SERV_STATUS_CONNECT == NVR_GetConnectStatus(R))
					{
						fl_color(m_cell_fgcolor);
						sprintf(s, "在线");		
					}
					break;
				default:
					break;
			}
		}else if(m_eTableType == E_VLIST_TABLE)
		{
			switch (C)
			{
				case 1:
					sprintf(s, "%d",R+1);	//序号
					break;
				case 2:
					if(R < int(m_FileString.size()))
					{
						const char *pFileName = strrchr(m_FileString[R].c_str(),'/');
						if(pFileName)
						{
							sprintf(s, "%s",pFileName+1);	//文件名
						}
						else
						{
							sprintf(s, "%s",m_FileString[R].c_str());	//文件名
						}
					}
					
					break;
				default:
					break;
			}
		}
			fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
			// BORDER
			fl_color(color()); 
			fl_rect(X, Y, W, H);
	    }
	    fl_pop_clip();
	    return;
	}
	case CONTEXT_TABLE:
	    fprintf(stderr, "TABLE CONTEXT CALLED\n");
	    return;

	case CONTEXT_ENDPAGE:
	case CONTEXT_NONE:
	    return;
    }
}

// Callback whenever someone clicks on different parts of the table
void MyTable::event_callback(Fl_Widget*, void *data)
{
    MyTable *o = (MyTable*)data;
    o->event_callback2();
}

void MyTable::event_callback2()
{
   

}

void MyTable::SetSize(int newrows, int newcols) {
    clear();		// clear any previous widgets, if any
    rows(newrows);
	if(newcols != cols())
	{
		cols(newcols);
	}

    begin();		// start adding widgets to group
    {
    	if(m_eTableType == E_VLIST_TABLE)
    	{
    		for ( int r = 0; r<newrows; r++ ) 
	  		{
				for ( int c = 0; c<newcols; c++ ) 
				{
	  				int X,Y,W,H;
	  				find_cell(CONTEXT_TABLE, r, c, X, Y, W, H);
	  				if ( 0 == c )
					{
						Fl_Check_Button *pCheckBtn = new Fl_Check_Button(X+10,Y,18,H);
						pCheckBtn->box(FL_FLAT_BOX);
						pCheckBtn->color(FL_WHITE);
      					//pCheckBtn->down_box(FL_FLAT_BOX);
      					//pCheckBtn->color(FL_WHITE);
      					//pCheckBtn->selection_color(FL_WHITE);
	  				}
				}
      		}
    	}  
    }
    end();
 }

void MyTable::SetTableTyle(Fl_TableType eType)
{
	m_eTableType = eType;
}

bool MyTable::GetFileSelectState(int iIndexFile)
{
	if ( iIndexFile >= children() || m_eTableType != E_VLIST_TABLE) 
	{
		return false;
	}
	Fl_Check_Button *pCheckBtn = (Fl_Check_Button *)child(iIndexFile);
	return pCheckBtn->value();
}

void MyTable::SetPlayBkColor(Fl_Color val)
{
	m_Playcolor = val;
}
void MyTable::SetPlayIndex(int iIndex)
{
	m_iPlayIndex = iIndex; 
}

int	 MyTable::GetPlayIndex()
{
    return m_iPlayIndex;
}

int MyTable::SetLogInfo(PT_MSG_LOG_INFO ptLog)
{
	if(m_pLogInfo)
	{
		free (m_pLogInfo);
	}
	m_pLogInfo = ptLog;
	return 0;
}

PT_MSG_LOG_INFO MyTable::GetLogInfo()
{
	if(!m_pLogInfo)
	{
		m_pLogInfo = (PT_MSG_LOG_INFO)malloc(MAX_LOG_NUMBER * sizeof(T_MSG_LOG_INFO));
	}
	return m_pLogInfo;
}

int MyTable::setLogNum(int iNum)
{
	m_iLogNum = iNum;
	return 0;
}
int MyTable::GetLogNum()
{
	return m_iLogNum;
}

int MyTable::SetLogPage(int iPage)
{
	m_iLogPage = iPage;
	return 0;
}

int MyTable::GetLogPage()
{
	return m_iLogPage;
}

int MyTable::SetLogType(int iType)//0 sys 1 Operator
{
	m_iLogType = iType;
	return 0;
}


/*int MyTable::ClearLogInfo()
{
	if(m_pLogInfo)
	{
		
	}
	return m_pLogInfo;
}*/



