#ifndef MYTABLE_H
#define MYTABLE_H

#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Table_Row.H>
#include <vector>
#include <map>
#include <string>
#include "log.h"

using namespace std;
// Simple demonstration class to derive from Fl_Table_Row
class MyTable : public Fl_Table_Row
{
public:
	enum Fl_TableType
	{
		E_LOG_TABLE,
		E_DISKST_TABLE,
		E_VLIST_TABLE
	};
	
private:
    Fl_Color m_cell_bgcolor;				// color of cell's bg color
    Fl_Color m_cell_fgcolor;				// color of cell's fg color
    Fl_Color m_Playcolor;
	Fl_TableType m_eTableType;
	int m_iPlayIndex;
protected:
    void draw_cell(TableContext context,  		// table cell drawing
    		   int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
    static void event_callback(Fl_Widget*, void*);
    void event_callback2();				// callback for table events
    

public:
    MyTable(int x, int y, int w, int h, const char *l=0) ;
	~MyTable();
    Fl_Color GetCellFGColor();
    Fl_Color GetCellBGColor();
    void SetCellFGColor(Fl_Color val);
    void SetCellBGColor(Fl_Color val);
	void SetTableTyle(Fl_TableType eType);
	void SetSize(int newrows, int newcols);
	bool GetFileSelectState(int iIndexFile);
	void SetPlayBkColor(Fl_Color val);
	void SetPlayIndex(int iIndex);
	int	 GetPlayIndex();
	int SetLogInfo(PT_MSG_LOG_INFO ptLog);
	PT_MSG_LOG_INFO GetLogInfo();
	int setLogNum(int iNum);
	int GetLogNum();
	int SetLogPage(int iPage);
	int GetLogPage();
	int SetLogType(int iType); //0 sys 1 Operator
	vector<string> m_FileString;
	PT_MSG_LOG_INFO m_pLogInfo;
	int	m_iLogNum;
	int m_iLogPage;
	int m_iLogType;
	
};
#endif

