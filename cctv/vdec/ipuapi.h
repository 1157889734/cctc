#ifndef __IPU_API_H__
#define __IPU_API_H__
#include "commondef.h"

typedef unsigned long IPU_HANDLE;

typedef int (*PF_VPU_CLR_DISP_CALLBACK)(void *pParam, int iDispIndex);

/*************************************************
	函数	:     ipu_init
	函数描述:     ipu初始化
	输入参数:     iScreenW iScreenH: 屏幕分辨率 
	输出参数:     无
	返回值:       0-成功，否则失败
*************************************************/
int		ipu_init(int iScreenW , int iScreenH );
//
/*************************************************
  函数	  :     ipu_unit
  函数描述:     ipu反初始化
  输入参数:     无
  输出参数:     无
  返回值:       无
*************************************************/
void	ipu_uninit();

///*************************************************
//  函数	:     ipu_createtask
//  函数描述:     创建IPU任务
//  输入参数:     iSrcWidth:解码后的分辨率宽值，iSrcHeiht：解码后的分辨率高值
//				  ptDstRect:图像显示的位置指针
//  输出参数:     无
//  返回值:       失败：0  成功：IPU返回的任务句柄
//*************************************************/
IPU_HANDLE ipu_createtask(int iSrcWidth,int iSrcHeiht,T_WINDOW_RECT *ptDstRect, PF_VPU_CLR_DISP_CALLBACK pfVpuClrDispCallback, void *pParam);

///*************************************************
//  函数	:     ipu_destroytask
//  函数描述:     将解码后的数据存到ipu队列里
//  输入参数:     hIpu:IPU任务句柄
//  输出参数:     无
//  返回值:      0-成功，否则失败
//*************************************************/
int		ipu_destroytask(IPU_HANDLE hIpu);

///*************************************************
//  函数	:     ipu_changePos
//  函数描述:    
//  输入参数:     hIpu:IPU任务句柄
//  输出参数:     无
//  返回值:      0-成功，否则失败
//*************************************************/
int		ipu_changePos(IPU_HANDLE hIpu,T_WINDOW_RECT *ptDstRect);

///*************************************************
//  函数	:     ipu_resChanged
//  函数描述:     分辨率发生改变
//  输入参数:     hIpu:IPU任务句柄
//  输出参数:     无
//  返回值:      0-成功，否则失败
//*************************************************/
int		ipu_reschanged(IPU_HANDLE hIpu,int iWidth,int iHeiht);

///*************************************************
//  函数	:     ipu_senddisaddr
//  函数描述:     传送显示的输入地址和地址索引号
//  输入参数:     hIpu:IPU任务句柄
//  输出参数:     无
//  返回值:      0-成功，否则失败
//*************************************************/
int		ipu_senddisaddr(IPU_HANDLE hIpu,unsigned long iPhyAddr,unsigned long iVirAddr,int iIndex);

///***********************************************
//  函数	:     ipu_setvdecindex
//  函数描述:     传送解码正在使用的地址索引号
//  输入参数:     hIpu:IPU任务句柄
//  输出参数:     无
//  返回值:      0-成功，否则失败
int		ipu_setvdecindex(IPU_HANDLE hIpu,int iIndex);

///***********************************************
//  函数	:     ipu_getdispindex
//  函数描述:     获取正在显示的索引
//  输入参数:     hIpu:IPU任务句柄
//  输出参数:     无
//  返回值:      0-成功，否则失败
int		ipu_getdispindex(IPU_HANDLE hIpu,int *pIndex);

int 	ipu_disableDisp(IPU_HANDLE hIpu);

int 	ipu_enableDisp(IPU_HANDLE hIpu);

int     ipu_setfb1Bkcolor(IPU_HANDLE hIpu,int x,int y,int w,int h);

#endif
