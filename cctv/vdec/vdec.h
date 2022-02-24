#ifndef		__VDEC_H__
#define		__VDEC_H__

typedef long VDEC_HADNDLE;

#ifdef  __cplusplus


extern "C"
{
#endif
/*************************************************
  函数功能:     VDEC_GetVersion
  函数描述:     获取版本号
  输入参数:     无
  输出参数:     无
  返回值:       版本号
  日期:         2016-12-5
  修改:   
*************************************************/
const char *VDEC_GetVersion();


/*************************************************
  函数功能:     VDEC_Init
  函数描述:     初使化视频解码
  输入参数:     iScreenWidth：屏宽，iScreenHeight：屏高
  输出参数:     无
  返回值:       0:成功, 否则:失败
  日期:         2018-06-5
  修改:   
*************************************************/
int VDEC_Init(int iScreenWidth, int iScreenHeight);

/*************************************************
  函数功能:     VDEC_Uninit
  函数描述:     反初使化视频解码
  输入参数:     无
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_UnInit(void);

/*************************************************
  函数功能:     VDEC_SetGuiBackground
  函数描述:     设置GUI背景色，使用时将播放窗口的背景色设置成此颜色
  输入参数:     uiColor：背景颜色
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_SetGuiBackground(unsigned int uiColor);

/*************************************************
  函数功能:     VDEC_SetGuiAlpha
  函数描述:     设置GUI透明度
  输入参数:     iAlpha: 透明度 （0-255）（0的时候fb0全透明）
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_SetGuiAlpha(int iAlpha);

/*************************************************
  函数功能:     VDEC_CreateVideoDecCh
  函数描述:     创建视频解码通道
  输入参数:     			
  输出参数:     无
  返回值:       返回视频解码句柄, 0-创建失败
  修改:   
*************************************************/
VDEC_HADNDLE VDEC_CreateVideoDecCh();

/*************************************************
  函数功能:     VDEC_DestroyVideoDecCh
  函数描述:     销毁视频解码通道
  输入参数:     VHandle:视频解码通道句柄
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_DestroyVideoDecCh(VDEC_HADNDLE VHandle);
/*************************************************
  函数功能:     VDEC_SetDecWindowsPos
  函数描述:     设置视频解码通道显示位置
  输入参数:     VHandle:视频解码通道句柄， x:起始点水平位置，y:起始点垂直位置，width:宽，height:高
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_SetDecWindowsPos(VDEC_HADNDLE VHandle, int x, int y, int width, int height);

/*************************************************
  函数功能:     VDEC_StartVideoDec
  函数描述:     开始视频解码
  输入参数:     VHandle:视频解码通道句柄， 
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_StartVideoDec(VDEC_HADNDLE VHandle);


/*************************************************
  函数功能:     VDEC_StopVideoDec
  函数描述:     停止视频解码
  输入参数:     VHandle:视频解码通道句柄， 
  输出参数:     无
  返回值:       0:成功, 否则:失败
  修改:   
*************************************************/
int VDEC_StopVideoDec(VDEC_HADNDLE VHandle);

/*************************************************
  函数功能:     VDEC_SendVideoStream
  函数描述:     发送视频解码数据
  输入参数:     VHandle:视频解码通道句柄，
  				pcFrame:视频解码数据地址
  				iLen:数据长度
  输出参数:     无
  返回值:       0:成功, 否则:失败
  日期:        
  修改:   
*************************************************/
int VDEC_SendVideoStream(VDEC_HADNDLE VHandle, char *pcFrame, int iLen);

int VDEC_VideoDecDisp(VDEC_HADNDLE VHandle,int iDispEnable,int iDecEnable);

int VDEC_GetVideoDecDispEnable(VDEC_HADNDLE VHandle,int *piVdecEnable,int *piDispEnable);

int VDEC_Setfb1BKColor(VDEC_HADNDLE VHandle,int x,int y,int w,int h);  

#ifdef  __cplusplus
}
#endif

#endif
