
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ipuapi.h"
#include <linux/mxcfb.h>
#include <pthread.h>
#include <linux/ipu.h>
#include <errno.h>

#include "commondef.h"

static int g_fb1 = -1;
static int g_fbIpu = -1;
static struct fb_var_screeninfo g_fbVar;
static struct fb_fix_screeninfo g_fbFix;
static void *g_fbBuf = NULL;
static pthread_t g_pThreadDispId=0;
static int g_iIpuRun =0;
static int g_iIpuShow =0;
static pthread_mutex_t g_IpuMutex;

#define DEBUG_IPU_1  DEBUG_LEVEL_5
#define DEBUG_IPU_2  DEBUG_LEVEL_6

typedef struct {
	struct ipu_task tIpuTask;
	pthread_t pthreadId;
	int iTaskExitFlag;
	int iInbufsize;
	unsigned long uiFirstPhyDispAddr;
	unsigned long uiSecPhyDispAddr;
	unsigned long uiFirstVirDispAddr;
	unsigned long uiSecVirDispAddr;
	int			  iFirstIndex;
	int			  iSecIndex;
	int			  iNowIndex;
	int			  iDecIndex;

#ifdef _WINDOWS_
	HANDLE hMutex;
	HANDLE hIndexMutex;
#else
	pthread_mutex_t hMutex;
	pthread_mutex_t hIndexMutex;
#endif
	PF_VPU_CLR_DISP_CALLBACK pfVpuClrDispCallback;
	void *pCbParam;
	int iDispFlag;      // 0:disable disp, 1:enable disp
	int fbIpu ;
} T_IPU_INFO;


static unsigned int fmt_to_bpp(unsigned int pixelformat)
{
	unsigned int bpp;

	switch (pixelformat)
	{
	case IPU_PIX_FMT_RGB565:
		/*interleaved 422*/
	case IPU_PIX_FMT_YUYV:
	case IPU_PIX_FMT_UYVY:
		/*non-interleaved 422*/
	case IPU_PIX_FMT_YUV422P:
	case IPU_PIX_FMT_YVU422P:
		bpp = 16;
		break;
	case IPU_PIX_FMT_BGR24:
	case IPU_PIX_FMT_RGB24:
	case IPU_PIX_FMT_YUV444:
	case IPU_PIX_FMT_YUV444P:
		bpp = 24;
		break;
	case IPU_PIX_FMT_BGR32:
	case IPU_PIX_FMT_BGRA32:
	case IPU_PIX_FMT_RGB32:
	case IPU_PIX_FMT_RGBA32:
	case IPU_PIX_FMT_ABGR32:
		bpp = 32;
		break;
		/*non-interleaved 420*/
	case IPU_PIX_FMT_YUV420P:
	case IPU_PIX_FMT_YVU420P:
	case IPU_PIX_FMT_YUV420P2:
	case IPU_PIX_FMT_NV12:
	case IPU_PIX_FMT_TILED_NV12:
		bpp = 12;
		break;
	default:
		bpp = 8;
		break;
	}
	return bpp;
}
void *ipu_display_loop(void *arg)
{
	//struct timeval tdec_begin,tdec_end;
	while(g_iIpuRun)
	{
		if(g_iIpuShow)
		{
			int iRet = ioctl(g_fb1, FBIOPAN_DISPLAY, &g_fbVar);
			//gettimeofday(&tdec_begin, NULL);
			if (iRet < 0) {
				err_msg( "fb ioct FBIOPAN_DISPLAY fail\n");
			}
			//gettimeofday(&tdec_end, NULL);
			//printf("DISPLAY cast Time = %d\n",
			//	(tdec_end.tv_sec - tdec_begin.tv_sec)*1000000 
			//	+tdec_end.tv_usec - tdec_begin.tv_usec);
		}
		usleep(40000);
	}
}


int ipu_init(int iScreenW/* = 1024*/,int iScreenH /*=768*/)
{
	int ifbSize = 0;
    
	if(g_fbIpu <0)
	{
		g_fbIpu = open("/dev/mxc_ipu", O_RDWR, 0);
		if (g_fbIpu < 0) {
		err_msg("open /dev/mxc_ipu fail\n");
		return -1;
		}
	}	
    
	if(g_fb1 <0)
	{
		g_fb1 = open("/dev/fb1", O_RDWR, 0);
		if (g_fb1 < 0) {
			err_msg("open /dev/fb1 fail\n");
			close(g_fbIpu);
			g_fbIpu = -1;
			return -1;
		}
	}
	ioctl(g_fb1, FBIOGET_VSCREENINFO, &g_fbVar);
	g_fbVar.xres = iScreenW;
	g_fbVar.xres_virtual = g_fbVar.xres;
	g_fbVar.yres = iScreenH;
	g_fbVar.yres_virtual = g_fbVar.yres;
	g_fbVar.activate |= FB_ACTIVATE_FORCE;
	g_fbVar.vmode |= FB_VMODE_YWRAP;
	g_fbVar.nonstd = v4l2_fourcc('I', '4','2', '0');
	g_fbVar.bits_per_pixel = fmt_to_bpp(v4l2_fourcc('I', '4','2', '0'));

	ifbSize = iScreenW * iScreenH * g_fbVar.bits_per_pixel /8;
	int iRet = ioctl(g_fb1, FBIOPUT_VSCREENINFO, &g_fbVar);
	if (iRet < 0) {
		err_msg("fb ioctl FBIOPUT_VSCREENINFO fail\n");
		//close(g_fbIpu);
		close(g_fb1);
		//g_fbIpu = -1;
		g_fb1 = -1;
		return -1;
	}
	ioctl(g_fb1, FBIOGET_VSCREENINFO, &g_fbVar);
	ioctl(g_fb1, FBIOGET_FSCREENINFO, &g_fbFix);
	int blank = FB_BLANK_UNBLANK;
	ioctl(g_fb1, FBIOBLANK, blank);
	g_fbBuf= mmap(0,ifbSize,PROT_READ|PROT_WRITE, MAP_SHARED,g_fb1, 0);  //这个参数只有在视频分辨率与显示位置大小相同时才会用到
	if(MAP_FAILED == g_fbBuf)
	{
		g_fbBuf = NULL;
		info_msg("mmap fbbuf  error\n");		
	}	
	g_iIpuRun = 1;
	g_iIpuShow = 0;
  //  pthread_mutex_init(&g_IpuMutex,NULL);  
	//pthread_create(&g_pThreadDispId,NULL, ipu_display_loop,(void *)NULL);	
	return 0;

}

void ipu_uninit()
{
	int blank = FB_BLANK_POWERDOWN;
	int ifbSize = g_fbVar.xres * g_fbVar.yres * g_fbVar.bits_per_pixel /8;
	g_iIpuRun = 0;
	//pthread_join(g_pThreadDispId, NULL);	
	ioctl(g_fb1, FBIOBLANK, blank);
	if (g_fbBuf)
		munmap(g_fbBuf, ifbSize);

	/*if(g_fbIpu>0)
	{
		close(g_fbIpu);
		g_fbIpu = -1;
	}*/
	if(g_fb1>0)
	{
		close(g_fb1);
		g_fb1 = -1;
	}
	
}
void *ipu_Task_loop(void *arg)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)arg;
	struct ipu_task *pTask = &ptIpuInfo->tIpuTask;
	unsigned long uiVirAddr = 0;
	//struct timeval tdec_begin,tdec_end;
	while(!ptIpuInfo->iTaskExitFlag)
	{
		int iRet = 0;
		if(0 == ptIpuInfo->uiFirstPhyDispAddr || (0 == ptIpuInfo->iDispFlag))
		{
			usleep(5000);
			continue;
		}

		pthread_mutex_lock( &ptIpuInfo->hIndexMutex );
		pTask->input.paddr = ptIpuInfo->uiFirstPhyDispAddr;
		ptIpuInfo->uiFirstPhyDispAddr = ptIpuInfo->uiSecPhyDispAddr;
		uiVirAddr = ptIpuInfo->uiFirstVirDispAddr;
		ptIpuInfo->uiFirstVirDispAddr = ptIpuInfo->uiSecVirDispAddr;
		ptIpuInfo->iNowIndex = ptIpuInfo->iFirstIndex;
		ptIpuInfo->iFirstIndex = ptIpuInfo->iSecIndex; 
		ptIpuInfo->uiSecPhyDispAddr = 0;
		ptIpuInfo->uiSecVirDispAddr = 0;
		ptIpuInfo->iSecIndex = -1; 
		
		pthread_mutex_unlock( &ptIpuInfo->hIndexMutex );

		if (0 == ptIpuInfo->iDispFlag)
		{
			if (ptIpuInfo->pfVpuClrDispCallback)
			{
				ptIpuInfo->pfVpuClrDispCallback(ptIpuInfo->pCbParam, ptIpuInfo->iNowIndex);
	 		}
			
			usleep(20000);
			continue;
		}
        iRet = ioctl(ptIpuInfo->fbIpu, IPU_CHECK_TASK, pTask);
        if (iRet != IPU_CHECK_OK) 
        {
            err_msg("IPU_CHECK_TASK fail\n");
        }
		
		pthread_mutex_lock( &ptIpuInfo->hMutex );
		if(pTask->input.width != pTask->output.crop.w || pTask->input.height != pTask->output.crop.h)
		{
			//gettimeofday(&tdec_begin, NULL);
			//pthread_mutex_lock( &g_IpuMutex);
			DebugPrint(DEBUG_IPU_2,"[%s:%d] ipu %p", __FUNCTION__, __LINE__, ptIpuInfo);
			iRet = ioctl(ptIpuInfo->fbIpu, IPU_QUEUE_TASK, pTask);
			if (iRet < 0) {
				pthread_mutex_unlock( &ptIpuInfo->hMutex );
				err_msg("ioct IPU_QUEUE_TASK fail\n");
                //pthread_mutex_unlock( &g_IpuMutex);
				continue;
			}
			DebugPrint(DEBUG_IPU_2,"[%s:%d] ipu %p", __FUNCTION__, __LINE__, ptIpuInfo);
           // pthread_mutex_unlock( &g_IpuMutex);
		//	gettimeofday(&tdec_end, NULL);
		//	printf("Task cast Time = %d\n",
		//		(tdec_end.tv_sec - tdec_begin.tv_sec)*1000000 
		//		+tdec_end.tv_usec - tdec_begin.tv_usec);			
		}
		else if( g_fbBuf )
		{
			int ileft =pTask->output.crop.pos.x ;
			int itop = pTask->output.crop.pos.y ;
			int iwidth = pTask->output.crop.w;
			int height = pTask->output.crop.h;
			int i = itop;
			int ibitCount = g_fbVar.bits_per_pixel;
			for(; i<itop +height ;i++)
			{
				memcpy(g_fbBuf + (ileft +i*g_fbVar.xres )
					,(void *)(uiVirAddr +iwidth *(i -itop))
					,iwidth);

				memcpy(g_fbBuf + (ileft+i*g_fbVar.xres)/4 + g_fbVar.xres * g_fbVar.yres 
					,(void *)(uiVirAddr +iwidth *(i -itop)/4 + iwidth * height)
					,iwidth/4);

				memcpy(g_fbBuf + (ileft+i*g_fbVar.xres)/4 + (g_fbVar.xres * g_fbVar.yres*5)/4 
					,(void *)(uiVirAddr +iwidth *(i -itop)/4 + (iwidth * height*5)/4)
					,iwidth/4);
			}
		}	
		if (ptIpuInfo->pfVpuClrDispCallback)
		{
			ptIpuInfo->pfVpuClrDispCallback(ptIpuInfo->pCbParam, ptIpuInfo->iNowIndex);
		}
		pthread_mutex_unlock( &ptIpuInfo->hMutex );
		usleep(2000);
	}

	return NULL;
}

IPU_HANDLE ipu_createtask(int iSrcWidth,int iSrcHeiht,T_WINDOW_RECT *ptDstRect, PF_VPU_CLR_DISP_CALLBACK pfVpuClrDispCallback, void *pParam)
{
	T_IPU_INFO* ptIpuInfo = NULL;
	struct ipu_task *pIpuTask =NULL;
	if(!iSrcWidth || !iSrcHeiht || !ptDstRect->iWidth || !ptDstRect->iHeight)
	{
		return 0;
	}
	//info_msg("iSrcWidth %d iSrcHeiht %d x = %d y =%d w = %d h = %d\n",iSrcWidth, iSrcHeiht,
	//	ptDstRect->iLeft,ptDstRect->iTop,ptDstRect->iWidth,ptDstRect->iHeight);
	ptIpuInfo = (T_IPU_INFO*)malloc(sizeof(T_IPU_INFO));
	if (NULL == ptIpuInfo)
	{
		err_msg( "malloc T_IPU_INFO error\n");
		return 0;
	}
	memset(ptIpuInfo,0,sizeof(T_IPU_INFO));

	ptIpuInfo->fbIpu = g_fbIpu;//open("/dev/mxc_ipu", O_RDWR, 0);
	if (ptIpuInfo->fbIpu < 0) 
    {
	    err_msg("open /dev/mxc_ipu fail\n");
        free(ptIpuInfo);
	    return 0;
	}
  
	pthread_mutex_init(&ptIpuInfo->hMutex,NULL);  
	pthread_mutex_init(&ptIpuInfo->hIndexMutex,NULL);  
	pIpuTask = &ptIpuInfo->tIpuTask;
	ptIpuInfo->iNowIndex = -1;
	/*default settings*/
	pIpuTask->priority = 0;
	pIpuTask->task_id = 0;
	pIpuTask->timeout = 2000;
	
	pIpuTask->input.width = iSrcWidth;
	pIpuTask->input.height = iSrcHeiht;
	pIpuTask->input.format =  v4l2_fourcc('I', '4','2', '0');
	pIpuTask->input.crop.pos.x = 0;
	pIpuTask->input.crop.pos.y = 0;
	pIpuTask->input.crop.w = 0;
	pIpuTask->input.crop.h = 0;
	pIpuTask->input.deinterlace.enable = 0;
	pIpuTask->input.deinterlace.motion = 0;

	pIpuTask->overlay_en = 0;
	pIpuTask->overlay.width = g_fbVar.xres;
	pIpuTask->overlay.height = g_fbVar.yres;
	pIpuTask->overlay.format = v4l2_fourcc('I', '4','2', '0');
	pIpuTask->overlay.crop.pos.x = 0;
	pIpuTask->overlay.crop.pos.y = 0;
	pIpuTask->overlay.crop.w = 0;
	pIpuTask->overlay.crop.h = 0;
	pIpuTask->overlay.alpha.mode = 0;
	pIpuTask->overlay.alpha.gvalue = 0;
	pIpuTask->overlay.colorkey.enable = 0;
	pIpuTask->overlay.colorkey.value = 0x555555;

	pIpuTask->output.width = g_fbVar.xres;
	pIpuTask->output.height = g_fbVar.yres;
	pIpuTask->output.format = v4l2_fourcc('I', '4','2', '0');
	pIpuTask->output.rotate = 0;
	pIpuTask->output.crop.pos.x = ptDstRect->iLeft;
	pIpuTask->output.crop.pos.y = ptDstRect->iTop;
	pIpuTask->output.crop.w = ptDstRect->iWidth;
	pIpuTask->output.crop.h = ptDstRect->iHeight;
	pIpuTask->output.paddr  = g_fbFix.smem_start ;

	ptIpuInfo->iInbufsize = pIpuTask->input.width * pIpuTask->input.height* fmt_to_bpp(pIpuTask->input.format)/8;
	ptIpuInfo->iTaskExitFlag = 0;
	ptIpuInfo->pfVpuClrDispCallback = pfVpuClrDispCallback;
	ptIpuInfo->pCbParam = pParam;
	ptIpuInfo->iDispFlag = 0;   //默认是不显示的
	pthread_create(&ptIpuInfo->pthreadId,NULL, ipu_Task_loop,(void *)ptIpuInfo);	

	DebugPrint(DEBUG_IPU_1,"[%s:%d] ipu %p, vpu %p", __FUNCTION__, __LINE__, ptIpuInfo, pParam);

	return (IPU_HANDLE)ptIpuInfo;

}
int ipu_destroytask(IPU_HANDLE hIpu)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;
	if(NULL == ptIpuInfo )
	{
		return -1;
	}
	ptIpuInfo->iTaskExitFlag = 1;
	pthread_join(ptIpuInfo->pthreadId, NULL);	
	pthread_mutex_destroy(&ptIpuInfo->hMutex);
	pthread_mutex_destroy(&ptIpuInfo->hIndexMutex);
    usleep(10*1000);
    if(ptIpuInfo->fbIpu>0)
	{
		//close(ptIpuInfo->fbIpu);
		ptIpuInfo->fbIpu = -1;
    }
    usleep(10*1000);
	free(ptIpuInfo);
	return 0;
}


int ipu_changePos(IPU_HANDLE hIpu,T_WINDOW_RECT *ptDstRect)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;
	struct ipu_task *pIpuTask =NULL;
	if(!ptDstRect->iWidth || !ptDstRect->iHeight)
	{
		return  -1;
	}
	if(NULL == ptIpuInfo )
	{
		return -1;
	}
	
	if((ptDstRect->iLeft + ptDstRect->iWidth) > g_fbVar.xres ||
		(ptDstRect->iTop + ptDstRect->iHeight) > g_fbVar.yres)
		return -1;

	pIpuTask = &ptIpuInfo->tIpuTask;
	pthread_mutex_lock( &ptIpuInfo->hMutex );
	pIpuTask->output.crop.pos.x = ptDstRect->iLeft;
	pIpuTask->output.crop.pos.y = ptDstRect->iTop;
	pIpuTask->output.crop.w = ptDstRect->iWidth;
	pIpuTask->output.crop.h = ptDstRect->iHeight;
	pthread_mutex_unlock( &ptIpuInfo->hMutex );
	return 0;
}

int ipu_senddisaddr(IPU_HANDLE hIpu,unsigned long iPhyAddr,unsigned long iVirAddr,int iIndex)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;
	if(NULL == ptIpuInfo )
	{
		return -1;
	}
    if(0 == ptIpuInfo->iDispFlag)
    {
        return 0;
    }
	DebugPrint(DEBUG_IPU_1,"[%s:%d] ipu %p", __FUNCTION__, __LINE__, ptIpuInfo);
	pthread_mutex_lock( &ptIpuInfo->hIndexMutex );
	if(0 == ptIpuInfo->uiFirstPhyDispAddr )
	{
		ptIpuInfo->uiFirstPhyDispAddr = iPhyAddr;
		ptIpuInfo->uiFirstVirDispAddr = iVirAddr;
		ptIpuInfo->iFirstIndex = iIndex;
	}
	else
	{
		if ((ptIpuInfo->uiSecPhyDispAddr) &&  (ptIpuInfo->pfVpuClrDispCallback))
		{
			// 还没有显示，直接覆盖需要释放此内存	
			ptIpuInfo->pfVpuClrDispCallback(ptIpuInfo->pCbParam, ptIpuInfo->iSecIndex);
            
		}
		//printf("UnNormal pfVpuClrDispCallback\n");
		ptIpuInfo->uiSecPhyDispAddr = iPhyAddr;
		ptIpuInfo->uiSecVirDispAddr = iVirAddr;
		ptIpuInfo->iSecIndex = iIndex;
        
	}
	pthread_mutex_unlock( &ptIpuInfo->hIndexMutex );
	return 0;
}


int ipu_setvdecindex(IPU_HANDLE hIpu,int iIndex)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;
	struct ipu_task *pIpuTask =NULL;
	if(NULL == ptIpuInfo )
	{
		return -1;
	}
	pthread_mutex_lock( &ptIpuInfo->hIndexMutex );
	ptIpuInfo->iDecIndex = iIndex;
	pthread_mutex_unlock( &ptIpuInfo->hIndexMutex );
	return 0;
}

int ipu_getdispindex(IPU_HANDLE hIpu,int *pIndex)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;
	struct ipu_task *pIpuTask =NULL;
	if(NULL == ptIpuInfo )
	{
		return -1;
	}
	pthread_mutex_lock( &ptIpuInfo->hIndexMutex );
	*pIndex = ptIpuInfo->iNowIndex;
	pthread_mutex_unlock( &ptIpuInfo->hIndexMutex );
	return 0;
}

int ipu_disableDisp(IPU_HANDLE hIpu)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;

	if(NULL == ptIpuInfo )
	{
		return -1;
	}

	ptIpuInfo->iDispFlag = 0;
	return 0;
}

int ipu_enableDisp(IPU_HANDLE hIpu)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;

	if(NULL == ptIpuInfo )
	{
		return -1;
	}

	ptIpuInfo->iDispFlag = 1;

	return 0;
}

int ipu_setfb1Bkcolor(IPU_HANDLE hIpu,int x,int y,int w,int h)
{
	T_IPU_INFO *ptIpuInfo = (T_IPU_INFO*)hIpu;
    info_msg("begin\n");
	if(g_fbBuf)
	{
		if(NULL != ptIpuInfo )
		{
			pthread_mutex_lock( &ptIpuInfo->hMutex );
		}
		
		int ibitCount = g_fbVar.bits_per_pixel;
		int i = y;
		for(;i<y +h && i<g_fbVar.yres ;i++)
		{
			memset(g_fbBuf + (x+i*g_fbVar.xres ),0,w);
			memset(g_fbBuf + x/2+(i/2)*(g_fbVar.xres/2) + g_fbVar.xres * g_fbVar.yres,128,w/2);
			memset(g_fbBuf + x/2+(i/2)*(g_fbVar.xres/2) + (g_fbVar.xres * g_fbVar.yres*5)/4,128,w/2);
		}
		if(NULL != ptIpuInfo )
		{
			pthread_mutex_unlock( &ptIpuInfo->hMutex );
		}
        info_msg("end\n");
		return 0;
	}	
    info_msg("end\n");
	err_msg("ipu_setfb1Bkcolor fail\n");	
	return -1;
}


