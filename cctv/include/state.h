#ifndef _STATE_H
#define _STATE_H


#define u8int		unsigned char
#define u16int		unsigned short
#define u32int		unsigned int

typedef enum _E_FILE_SEARCH_STATE
{
	E_FILE_IDLE =0,
	E_FILE_SEARCHING,
	E_FILE_FINISH
}E_FILE_SEARCH_STATE;

typedef enum _E_SERV_CONNECT_STATUS
{
    E_SERV_STATUS_UNCONNECT	= 0,
    E_SERV_STATUS_CONNECT = 1
}E_SERV_CONNECT_STATUS;

typedef enum _E_FILE_DOWN_STATE
{
	E_FILE_DOWN_IDLE =0,
	E_FILE_DOWNING,
	E_FILE_DOWN_SUCC,
	E_FILE_DOWN_FAIL,
}E_FILE_DOWN_STATE;

typedef enum _E_VIDEO_SOURCE_STATE
{
	E_SOURCE_IDLE = 0x11,
	E_SOURCE_WAIT = 0x12,
	E_SOURCE_SEND_FAIL = 0x13,
	E_SOURCE_SET_SUCC  = 0x14,
	E_SOURCE_ALIVE_FAIL = 0x15,
	E_SOURCE_DIGTV_FAIL = 0x16,
	E_SOURCE_LOCAL_FAIL	= 0x17
}E_VIDEO_SOURCE_STATE;

typedef enum
{
	E_UNKONE_VPLAY =	0,
	E_SINGLE_VPLAY = 1,
	E_FOUR_VPLAY	= 2,
}E_PLAY_STYLE;

typedef enum
{
	E_RES_UPDATE_UNKNOW= 0,
	E_RES_UPDATE_START = 1,
	E_RES_UPDATE_ING   = 2,
	E_RES_UPDATE_SUCC  = 3,
	E_RES_UPDATE_FAIL  = 4	
}E_RES_UPDATE_STATE;

typedef struct _T_DISK_STATE
{
    char  cDiskNum;
    short sDiskTotalSize;
    short sDiskUseSize;	
    char  cDiskLostWarn;
    char  cDiskSizeWarn;
} __attribute__((packed)) T_DISK_STATE;

typedef struct _T_NVR_STATE
{
    int iNVRConnectState;
	char acIp[16];
	T_DISK_STATE tDiskState;	
	int iVideoNum;
	char acVideoIdx[12];
}__attribute__((packed))T_NVR_STATE, *PT_NVR_STATE;

enum _E_DISP_STATE
{
    DISP_STATE_UNKOWN = 0,
    DISP_STATE_DMI,
    DISP_STATE_HMI,
    DISP_STATE_CCTV,
};

#ifdef  __cplusplus
extern "C"
{
#endif

//设置CCTV的运行目录
int SetCCTVRunDir(const char *pcData,int iLen);
//获取CCTV的运行目录
int GetCCTVRunDir(char *pcRunData,int iLen);

//初始化前先设置运行目录
int STATE_Init(void);
int STATE_Uninit(void);


//******************CCTV屏*******************
//设备IP
int GetDeviceIp(char *pcIp,int ilen);
//设备车号 返回值：1 或者6
int GetDeviceCarriageNo();

//******************相机状态******************
//相机数量
int GetVideoNum();

//相机的Rtsp地址
int SetVideoRtspUrl(int iVideoIdx, char *pcUrl,int iUrlLen);
int GetVideoRtspUrl(int iVideoIdx, char *pcUrl, int iUrlLen);
int GetVideoMainRtspUrl(int iVideoIdx, char *pcUrl, int iUrlLen);


//相机在线状态 1在线 0离线
int SetVideoOnlineState(int iVideoIdx, int iState);
int GetVideoOnlineState(int iVideoIdx);

//根据按钮位置获取相机的序列号
int GetVideoIdxAccordBtnPose(int iGroup,int iPos); 

//根据相机序号获取按钮的位置
int GetBtnPoseAccordVideoIdx(int iIndex,int *pGroup,int *iPos);

//获取NVR相关的相机数量
int GetNvrVideoNum(int iNvrNo);

//获取NVR第几相机的相机序号
int GetNvrVideoIdx(int iNvrNo,int iVideoNo);

//获取相机的名称
int GetVideoName(int iVideoIdx,char *pName,int iLen);

//相机的报警状态
int SetVideoWarnState(int iVideoIdx, int iState);
int GetVideoWarnState(int iVideoIdx);

//相机的图片序号
int GetVideoImgIdx(int iVideoIdx);

//获取相机的Nvr通道号
int GetVideoNvrNo(int iVideoIdx);



//********************NVR相关信息*****************
//获取所有的Nvr信息
int GetAllNvrInfo(T_NVR_STATE *pData,int iLen);

//NvrIp地址 
int SetNvrIpAddr(int iNvrNo,char *pIp);
int GetNvrIpAddr(int iNvrNo,char *pIp);

//Nvr硬盘
int SetNvrDiskState(int iNvrNo,T_DISK_STATE *ptDiskState);
int GetNvrDiskState(int iNvrNo,T_DISK_STATE *ptDiskState);
int GetNvrDiskNum(int iNvrNo);
int GetNvrDiskTotalSize(int iNvrNo);
int GetNvrDiskUsedSize(int iNvrNo);

//-1:获取失败 0:正常 1:丢失 2:容量不足
int GetNvrDiskWarnState(int iNvrNo);  



//******************文件搜索 文件下载******************
int SetFileSearchState(E_FILE_SEARCH_STATE iState);
E_FILE_SEARCH_STATE GetFileSearchState();
int SetFileDownState(E_FILE_DOWN_STATE eState);  
E_FILE_DOWN_STATE GetFileDownState( );
int SetFileDownProgress(int iProgress);
int GetFileDownProgress();
int STATE_FindUsbDev();


//********************音频管理**********************************
int SetASCUMasterSlaveFlag();
int GetASCUMasterSlaveFlag();

int SetDriverRoomMonitorFlag(int iFlag);
int GetDriverRoomMonitorFlag();

int SetNoiseMonitorFlag(int iEnable);
int GetNoiseMonitorFlag();

int SetLcdVolumeValue(int iValue);
int GetLcdVolumeValue();

int SetDriverRoomSpeakerVolume(int iValue);  //司机监控室
int GetDriverRoomSpeakerVolume();

int SetCarriageSpeakerVolume(int iValue);  //客室 ?
int GetCarriageSpeakerVolume();

int SetDynamicMapBrightness(int iValue);
int GetDynamicMapBrightness();

int SetVideoSource(int iVideoType);  //0x11 直播 0x12 数字电视 0x13 录播
int GetVideoSource();


//************************报警管理*********************************
//第iNo个Pecu对应的相机索引          从0开始
int GetPecuVideoIndex(int iPecuIdx);
//Pecu的报警信息（24个PECU报警，最低位代表第1个PECU,依次类推代表24个PECU）
int SetPecuWarnInfo(u32int iValue);  
u32int GetPecuWarnInfo();
//最先发起PECU报警的相机
int SetPecuFirstWarnVideoIdx(int iVideoIdx);
int GetPecuFirstWarnVideoIdx();

int GetFireWarnVideoIdx(char iCarriageNo);
int SetFireWarnInfo(char iCarriageNo,u8int cWarnState);
int	GetFireWarnInfo(char iCarriageNo,u8int *pcWarnState);
int GetAllFireWarnInfo(char *pFireWarn,int iDataLen);

int GetAllDoorClipInfo(char *pDoorClipWarn,int iDataLen);
int SetAllDoorClipInfo(char *pDoorClipWarn,int iDataLen);

int GetDoorWarnVideoIdx(char iCarriageNo,int iNo);
int SetDoorWarnInfo(char iCarriageNo,u8int cWarnState);
int GetDoorWarnInfo(char iCarriageNo,u8int *pcWarnState);
int GetAllDoorWarnInfo(char *pDoorWarn,int iDataLen);

//00:与更新客户端无连接 01:开始更新 02：更新中 03：更新结束 04：更新失败 
int SetResUpdateState(char cState,char cProgress);
int GetResUpdateState(char *pcState,char *pcProgress);

//0 : DISP_STATE_UNKOWN 1:DISP_STATE_DMI 2:DISP_STATE_HMI 3:DISP_STATE_CCTV
void SetDisplayState(int iState);
int  GetDisplayState(void);

//1:只显示DMI 2:只显示HMI 3:CCTV
int GetDevDispMode();
int GetTestEnableFlg(char * pcFlg);

int GetCycTime();

int ExecSysCmd(char *cmd, char *result, int len);

#ifdef  __cplusplus
}
#endif
#endif

