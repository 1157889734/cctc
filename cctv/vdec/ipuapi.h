#ifndef __IPU_API_H__
#define __IPU_API_H__
#include "commondef.h"

typedef unsigned long IPU_HANDLE;

typedef int (*PF_VPU_CLR_DISP_CALLBACK)(void *pParam, int iDispIndex);

/*************************************************
	����	:     ipu_init
	��������:     ipu��ʼ��
	�������:     iScreenW iScreenH: ��Ļ�ֱ��� 
	�������:     ��
	����ֵ:       0-�ɹ�������ʧ��
*************************************************/
int		ipu_init(int iScreenW , int iScreenH );
//
/*************************************************
  ����	  :     ipu_unit
  ��������:     ipu����ʼ��
  �������:     ��
  �������:     ��
  ����ֵ:       ��
*************************************************/
void	ipu_uninit();

///*************************************************
//  ����	:     ipu_createtask
//  ��������:     ����IPU����
//  �������:     iSrcWidth:�����ķֱ��ʿ�ֵ��iSrcHeiht�������ķֱ��ʸ�ֵ
//				  ptDstRect:ͼ����ʾ��λ��ָ��
//  �������:     ��
//  ����ֵ:       ʧ�ܣ�0  �ɹ���IPU���ص�������
//*************************************************/
IPU_HANDLE ipu_createtask(int iSrcWidth,int iSrcHeiht,T_WINDOW_RECT *ptDstRect, PF_VPU_CLR_DISP_CALLBACK pfVpuClrDispCallback, void *pParam);

///*************************************************
//  ����	:     ipu_destroytask
//  ��������:     �����������ݴ浽ipu������
//  �������:     hIpu:IPU������
//  �������:     ��
//  ����ֵ:      0-�ɹ�������ʧ��
//*************************************************/
int		ipu_destroytask(IPU_HANDLE hIpu);

///*************************************************
//  ����	:     ipu_changePos
//  ��������:    
//  �������:     hIpu:IPU������
//  �������:     ��
//  ����ֵ:      0-�ɹ�������ʧ��
//*************************************************/
int		ipu_changePos(IPU_HANDLE hIpu,T_WINDOW_RECT *ptDstRect);

///*************************************************
//  ����	:     ipu_resChanged
//  ��������:     �ֱ��ʷ����ı�
//  �������:     hIpu:IPU������
//  �������:     ��
//  ����ֵ:      0-�ɹ�������ʧ��
//*************************************************/
int		ipu_reschanged(IPU_HANDLE hIpu,int iWidth,int iHeiht);

///*************************************************
//  ����	:     ipu_senddisaddr
//  ��������:     ������ʾ�������ַ�͵�ַ������
//  �������:     hIpu:IPU������
//  �������:     ��
//  ����ֵ:      0-�ɹ�������ʧ��
//*************************************************/
int		ipu_senddisaddr(IPU_HANDLE hIpu,unsigned long iPhyAddr,unsigned long iVirAddr,int iIndex);

///***********************************************
//  ����	:     ipu_setvdecindex
//  ��������:     ���ͽ�������ʹ�õĵ�ַ������
//  �������:     hIpu:IPU������
//  �������:     ��
//  ����ֵ:      0-�ɹ�������ʧ��
int		ipu_setvdecindex(IPU_HANDLE hIpu,int iIndex);

///***********************************************
//  ����	:     ipu_getdispindex
//  ��������:     ��ȡ������ʾ������
//  �������:     hIpu:IPU������
//  �������:     ��
//  ����ֵ:      0-�ɹ�������ʧ��
int		ipu_getdispindex(IPU_HANDLE hIpu,int *pIndex);

int 	ipu_disableDisp(IPU_HANDLE hIpu);

int 	ipu_enableDisp(IPU_HANDLE hIpu);

int     ipu_setfb1Bkcolor(IPU_HANDLE hIpu,int x,int y,int w,int h);

#endif
