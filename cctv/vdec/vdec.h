#ifndef		__VDEC_H__
#define		__VDEC_H__

typedef long VDEC_HADNDLE;

#ifdef  __cplusplus


extern "C"
{
#endif
/*************************************************
  ��������:     VDEC_GetVersion
  ��������:     ��ȡ�汾��
  �������:     ��
  �������:     ��
  ����ֵ:       �汾��
  ����:         2016-12-5
  �޸�:   
*************************************************/
const char *VDEC_GetVersion();


/*************************************************
  ��������:     VDEC_Init
  ��������:     ��ʹ����Ƶ����
  �������:     iScreenWidth������iScreenHeight������
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  ����:         2018-06-5
  �޸�:   
*************************************************/
int VDEC_Init(int iScreenWidth, int iScreenHeight);

/*************************************************
  ��������:     VDEC_Uninit
  ��������:     ����ʹ����Ƶ����
  �������:     ��
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_UnInit(void);

/*************************************************
  ��������:     VDEC_SetGuiBackground
  ��������:     ����GUI����ɫ��ʹ��ʱ�����Ŵ��ڵı���ɫ���óɴ���ɫ
  �������:     uiColor��������ɫ
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_SetGuiBackground(unsigned int uiColor);

/*************************************************
  ��������:     VDEC_SetGuiAlpha
  ��������:     ����GUI͸����
  �������:     iAlpha: ͸���� ��0-255����0��ʱ��fb0ȫ͸����
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_SetGuiAlpha(int iAlpha);

/*************************************************
  ��������:     VDEC_CreateVideoDecCh
  ��������:     ������Ƶ����ͨ��
  �������:     			
  �������:     ��
  ����ֵ:       ������Ƶ������, 0-����ʧ��
  �޸�:   
*************************************************/
VDEC_HADNDLE VDEC_CreateVideoDecCh();

/*************************************************
  ��������:     VDEC_DestroyVideoDecCh
  ��������:     ������Ƶ����ͨ��
  �������:     VHandle:��Ƶ����ͨ�����
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_DestroyVideoDecCh(VDEC_HADNDLE VHandle);
/*************************************************
  ��������:     VDEC_SetDecWindowsPos
  ��������:     ������Ƶ����ͨ����ʾλ��
  �������:     VHandle:��Ƶ����ͨ������� x:��ʼ��ˮƽλ�ã�y:��ʼ�㴹ֱλ�ã�width:��height:��
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_SetDecWindowsPos(VDEC_HADNDLE VHandle, int x, int y, int width, int height);

/*************************************************
  ��������:     VDEC_StartVideoDec
  ��������:     ��ʼ��Ƶ����
  �������:     VHandle:��Ƶ����ͨ������� 
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_StartVideoDec(VDEC_HADNDLE VHandle);


/*************************************************
  ��������:     VDEC_StopVideoDec
  ��������:     ֹͣ��Ƶ����
  �������:     VHandle:��Ƶ����ͨ������� 
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  �޸�:   
*************************************************/
int VDEC_StopVideoDec(VDEC_HADNDLE VHandle);

/*************************************************
  ��������:     VDEC_SendVideoStream
  ��������:     ������Ƶ��������
  �������:     VHandle:��Ƶ����ͨ�������
  				pcFrame:��Ƶ�������ݵ�ַ
  				iLen:���ݳ���
  �������:     ��
  ����ֵ:       0:�ɹ�, ����:ʧ��
  ����:        
  �޸�:   
*************************************************/
int VDEC_SendVideoStream(VDEC_HADNDLE VHandle, char *pcFrame, int iLen);

int VDEC_VideoDecDisp(VDEC_HADNDLE VHandle,int iDispEnable,int iDecEnable);

int VDEC_GetVideoDecDispEnable(VDEC_HADNDLE VHandle,int *piVdecEnable,int *piDispEnable);

int VDEC_Setfb1BKColor(VDEC_HADNDLE VHandle,int x,int y,int w,int h);  

#ifdef  __cplusplus
}
#endif

#endif
