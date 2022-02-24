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
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

#include "mxc_ipu_hl_lib.h"
#include "ipuapi.h"
#include "vpu_lib.h"
#include "vpu_io.h"
#include "vdec.h"
#include "linux/mxcfb.h"
#include "fb.h"

#define		MAX_DEC_RES_W	1920
#define		MAX_DEC_RES_H	1080

typedef struct {
	T_PACKET_QUEUE tPacketQueue;
    T_PACKET_QUEUE tPackIpu;
	pthread_t pthreadId;
	int iTaskExitFlag;
	IPU_HANDLE hIpuHandel;
	T_WINDOW_RECT tOutRect;
	DecHandle hDecHandle;
	vpu_mem_desc mem_desc;
	vpu_mem_desc ps_mem_desc;
	vpu_mem_desc slice_mem_desc;
	int iLastPicWidth;
	int iLastPicHeight;
	int iPicWidth;
	int iPicHeight;
	int iStride;
	int iRegfbcount;
	FrameBuffer *fb;
	struct frame_buf **pfbpool;
	int iDispEnable;  //默认不显示
	int iDecState;
	int iExistIFrame;
    int iHaveParseAndAlloc;   //0未解析 1已解析未分配
} T_VDEC_INFO;

enum _E_DEC_STATE
{
    E_NO_DEC = 0x0,
    E_DEC_START = 0x1,
    E_DEC_STARTING = 0x2,
    E_DEC_DISP = 0x3,     // 
    E_DEC_NO_DISP = 0x4,
};

static char acVersion[32] = "V1.0.0 build20180616";
static pthread_mutex_t g_hAllocMutex;


const char * VDEC_GetVersion()
{
	return acVersion;
}

int VDEC_Init(int iScreenWidth, int iScreenHeight)
{
	framebuf_init();
	int iErr = vpu_Init(NULL);
	if (iErr) {
		err_msg("VPU Init Failure.\n");
		return -1;
	}
    pthread_mutex_init(&g_hAllocMutex,NULL);  
	return ipu_init(iScreenWidth,iScreenHeight);
}

int VDEC_UnInit(void)
{
	vpu_UnInit();
	ipu_uninit();
    pthread_mutex_destroy(&g_hAllocMutex );  
	return 0;
}

int VDEC_SetGuiBackground(unsigned int uiColor)
{
	int fd_fb = open("/dev/fb0", O_RDWR, 0);
	struct mxcfb_color_key clrkey;
	if (fd_fb < 0) {
		err_msg("unable to open fb0\n");
		return -1;
	}
	clrkey.color_key = uiColor;
	clrkey.enable = 1;
	if (ioctl(fd_fb, MXCFB_SET_CLR_KEY, &clrkey) < 0) {
		err_msg("set color key failed\n");
		close(fd_fb);
		return -1;
	}
	close(fd_fb);
	return 0;
}

int VDEC_SetGuiAlpha(int iAlpha)
{
	int fd_fb = open("/dev/fb0", O_RDWR, 0);
	struct mxcfb_gbl_alpha alpha;
	if (fd_fb < 0) {
		err_msg("unable to open fb0\n");
		return -1;
	}
	alpha.alpha = iAlpha;
	alpha.enable = 1;
	if (ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
		err_msg("set alpha blending failed\n");
		close(fd_fb);
		return -1;
	}
	close(fd_fb);
	return 0;
}

static int decoder_open(T_VDEC_INFO *ptVpuInfo)
{
	if (NULL == ptVpuInfo)
	{
		return -1;
	}
	RetCode ret = RETCODE_FAILURE;
	DecHandle handle = {0};
	DecOpenParam oparam;
	
	memset(&oparam, 0, sizeof(oparam));
	oparam.bitstreamFormat = STD_AVC;
	oparam.bitstreamBuffer = ptVpuInfo->mem_desc.phy_addr;
	oparam.bitstreamBufferSize = STREAM_BUF_SIZE;
	oparam.pBitStream = (Uint8 *)ptVpuInfo->mem_desc.virt_uaddr;
	oparam.reorderEnable = 1;
	oparam.mp4DeblkEnable = 0;
	oparam.chromaInterleave = 0;
	oparam.mp4Class = 0;
	if (cpu_is_mx6x())
		oparam.avcExtension = 0;
	oparam.mjpg_thumbNailDecEnable = 0;
	oparam.mapType = 0;
	oparam.tiled2LinearEnable = 0;
	oparam.bitstreamMode = 1;
	oparam.jpgLineBufferMode =0;
	oparam.psSaveBuffer = ptVpuInfo->ps_mem_desc.phy_addr;
	oparam.psSaveBufferSize = PS_SAVE_SIZE;
	ret = vpu_DecOpen(&handle, &oparam);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecOpen failed, ret:%d\n", ret);
		return -1;
	}

	ptVpuInfo->hDecHandle = handle;
	return 0;
}

static void decoder_close(T_VDEC_INFO *ptVpuInfo)
{
	RetCode ret = RETCODE_FAILURE;
	ret = vpu_DecClose(ptVpuInfo->hDecHandle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_SWReset(ptVpuInfo->hDecHandle, 0);
		ret = vpu_DecClose(ptVpuInfo->hDecHandle);
		if (ret != RETCODE_SUCCESS)
			err_msg("vpu_DecClose failed\n");
	}
}

static int decoder_parse(T_VDEC_INFO *ptVpuInfo)
{
	if (NULL == ptVpuInfo)
	{
		return -1;
	}
	DecInitialInfo initinfo = {0};
	DecHandle handle = ptVpuInfo->hDecHandle;
	int align, extended_fbcount;
	RetCode ret =RETCODE_FAILURE ;
	char *count=NULL;

	/*
	 * If userData report is enabled, buffer and comamnd need to be set
 	 * before DecGetInitialInfo for MJPG.
	 */

	/* Parse bitstream and get width/height/framerate etc */
	vpu_DecSetEscSeqInit(handle, 1);
	ret = vpu_DecGetInitialInfo(handle, &initinfo);
	vpu_DecSetEscSeqInit(handle, 0);
	if (ret != RETCODE_SUCCESS) {
	//	err_msg("vpu_DecGetInitialInfo failed, ret:%d, errorcode:%u\n",
	//	         ret, initinfo.errorcode);
		return -1;
	}

	if (initinfo.streamInfoObtained) {

		//info_msg("H.264 Profile: %d Level: %d Interlace: %d\n",
		//	initinfo.profile, initinfo.level, initinfo.interlace);
		if (initinfo.aspectRateInfo) {
			int aspect_ratio_idc;
			int sar_width, sar_height;
			if ((initinfo.aspectRateInfo >> 16) == 0) {
				aspect_ratio_idc = (initinfo.aspectRateInfo & 0xFF);
				info_msg("aspect_ratio_idc: %d\n", aspect_ratio_idc);
			} else {
				sar_width = (initinfo.aspectRateInfo >> 16) & 0xFFFF;
				sar_height = (initinfo.aspectRateInfo & 0xFFFF);
				//info_msg("sar_width: %d, sar_height: %d\n",
				//	sar_width, sar_height);
			}
		} else {
			//info_msg("Aspect Ratio is not present.\n");
		}
	}

	
	//info_msg("Decoder: width = %d, height = %d, frameRateRes = %u, frameRateDiv = %u, minFrameBufferCount = %d\n",
	//		initinfo.picWidth, initinfo.picHeight,
	///		initinfo.frameRateRes, initinfo.frameRateDiv,
	//		initinfo.minFrameBufferCount);


	/*
	 * We suggest to add two more buffers than minFrameBufferCount:
	 *
	 * vpu_DecClrDispFlag is used to control framebuffer whether can be
	 * used for decoder again. One framebuffer dequeue from IPU is delayed
	 * for performance improvement and one framebuffer is delayed for
	 * display flag clear.
	 *
	 * Performance is better when more buffers are used if IPU performance
	 * is bottleneck.
	 *
	 * Two more buffers may be needed for interlace stream from IPU DVI view
	 */
	count = getenv("VPU_EXTENDED_BUFFER_COUNT");
	if (count)
		extended_fbcount = atoi(count);
	else
		extended_fbcount = 2;

	if (initinfo.interlace)
		ptVpuInfo->iRegfbcount = initinfo.minFrameBufferCount + extended_fbcount + 2;
	else
		ptVpuInfo->iRegfbcount = initinfo.minFrameBufferCount + extended_fbcount;
	/*if( ptVpuInfo->iRegfbcount>10)
	{
		ptVpuInfo->iRegfbcount = 10;
	}
    if(initinfo.minFrameBufferCount > ptVpuInfo->iRegfbcount)
	ptVpuInfo->iRegfbcount = initinfo.minFrameBufferCount;*/

    if( ptVpuInfo->iRegfbcount<=5)
	{
		ptVpuInfo->iRegfbcount +=2;
	}

	ptVpuInfo->iPicWidth = ((initinfo.picWidth + 15) & ~15);

	align = 16;
	if (initinfo.interlace == 1)
		align = 32;
	ptVpuInfo->iPicHeight = ((initinfo.picHeight + align - 1) & ~(align - 1));

	if ((ptVpuInfo->iPicWidth == 0) || (ptVpuInfo->iPicHeight == 0))
		return -1;
	ptVpuInfo->iLastPicWidth  = 0;
	ptVpuInfo->iLastPicHeight = 0;
	/*
	 * Information about H.264 decoder picture cropping rectangle which
	 * presents the offset of top-left point and bottom-right point from
	 * the origin of frame buffer.
	 *
	 * By using these four offset values, host application can easily
	 * detect the position of target output window. When display cropping
	 * is off, the cropping window size will be 0.
	 *
	 * This structure for cropping rectangles is only valid for H.264
	 * decoder case.
	 */
	/* worstSliceSize is in kilo-byte unit */
	ptVpuInfo->slice_mem_desc.size = initinfo.worstSliceSize * 1024;
	ptVpuInfo->iStride = ptVpuInfo->iPicWidth;
    printf("interlace:%d iRegfbcount=%d min:%d \n ",
        initinfo.interlace,ptVpuInfo->iRegfbcount,
        initinfo.minFrameBufferCount ,extended_fbcount);
	/* Allocate memory for frame status, Mb and Mv report */

	return 0;
}

static int	decoder_allocate_framebuffer(T_VDEC_INFO *ptVpuInfo)
{
	DecBufInfo bufinfo;
	int i, totalfb =ptVpuInfo->iRegfbcount;
	RetCode ret;
	DecHandle handle = ptVpuInfo->hDecHandle;
	FrameBuffer *fb;
	struct frame_buf **pfbpool;
	int stride;
	int delay = -1;

	/*
	 * 1 extra fb for deblocking on MX32, no need extrafb for blocking on MX37 and MX51
	 * dec->cmdl->deblock_en has been cleared to zero after set it to oparam.mp4DeblkEnable
	 * in decoder_open() function on MX37 and MX51.
	 */
	fb = ptVpuInfo->fb =(FrameBuffer *) calloc(totalfb, sizeof(FrameBuffer));
	if (fb == NULL) {
		err_msg("Failed to allocate fb\n");
		return -1;
	}
	pfbpool = ptVpuInfo->pfbpool = (struct frame_buf **)calloc(totalfb, sizeof(struct frame_buf *));
	if (pfbpool == NULL) {
		err_msg("Failed to allocate pfbpool\n");
		free(ptVpuInfo->fb);
		ptVpuInfo->fb = NULL;
		return -1;
	}
    for (i = 0; i < totalfb; i++) {
		pfbpool[i] = NULL	;	
	}

	//这里本该是应该分配实际的大小，但是考虑到分辨率可能会变化，所以按最大分辨率分配
	/* All buffers are linear */
	for (i = 0; i < totalfb; i++) {
		pfbpool[i] = framebuf_alloc(STD_AVC, 0,
			ptVpuInfo->iStride, ptVpuInfo->iPicHeight, 1);
		//pfbpool[i] = framebuf_alloc(STD_AVC, 0,
		//	MAX_DEC_RES_W, MAX_DEC_RES_H, 0);
		if (pfbpool[i] == NULL)		{
			err_msg("framebuf_alloc err i =%d\n",i);
			goto err;
		}	
	}
	for (i = 0; i < totalfb; i++) {
		fb[i].myIndex = i;
		fb[i].bufY = pfbpool[i]->addrY;
		fb[i].bufCb = pfbpool[i]->addrCb;
		fb[i].bufCr = pfbpool[i]->addrCr;
		fb[i].bufMvCol = pfbpool[i]->mvColBuf;
		
	}
	stride = ((ptVpuInfo->iStride + 15) & ~15);
	bufinfo.avcSliceBufInfo.bufferBase = ptVpuInfo->slice_mem_desc.phy_addr;
	bufinfo.avcSliceBufInfo.bufferSize = ptVpuInfo->slice_mem_desc.size;


	/* User needs to fill max suported macro block value of frame as following*/
	bufinfo.maxDecFrmInfo.maxMbX = ptVpuInfo->iStride / 16;
	bufinfo.maxDecFrmInfo.maxMbY = ptVpuInfo->iPicHeight / 16;
	bufinfo.maxDecFrmInfo.maxMbNum = ptVpuInfo->iStride * ptVpuInfo->iPicHeight / 256;

	/* For H.264, we can overwrite initial delay calculated from syntax.
	 * delay can be 0,1,... (in unit of frames)
	 * Set to -1 or do not call this command if you don't want to overwrite it.
	 * Take care not to set initial delay lower than reorder depth of the clip,
	 * otherwise, display will be out of order. */
	vpu_DecGiveCommand(handle, DEC_SET_FRAME_DELAY, &delay);

	ret = vpu_DecRegisterFrameBuffer(handle, fb, ptVpuInfo->iRegfbcount, stride, &bufinfo);
	if (ret != RETCODE_SUCCESS) {
		err_msg("Register frame buffer failed, ret=%d\n", ret);
		goto err;
	}
	return 0;

err:
	for (i = 0; i < totalfb; i++) {
		framebuf_free(pfbpool[i],ptVpuInfo->iStride, ptVpuInfo->iPicHeight);
	}

	free(ptVpuInfo->fb);
	free(ptVpuInfo->pfbpool);
	ptVpuInfo->fb = NULL;
	ptVpuInfo->pfbpool = NULL;
	return -1;
}

void decoder_free_framebuffer(T_VDEC_INFO *ptVpuInfo)
{
	int i = 0, totalfb = ptVpuInfo->iRegfbcount;

	for (i = 0; i < totalfb; i++) {
		if (ptVpuInfo->pfbpool)
		{
            framebuf_free(ptVpuInfo->pfbpool[i],ptVpuInfo->iStride, ptVpuInfo->iPicHeight);
			ptVpuInfo->pfbpool[i] = NULL;
		}
	}
	ptVpuInfo->iRegfbcount = 0;
	if (ptVpuInfo->fb) {
		free(ptVpuInfo->fb);
		ptVpuInfo->fb = NULL;
	}
	if (ptVpuInfo->pfbpool) {
		free(ptVpuInfo->pfbpool);
		ptVpuInfo->pfbpool = NULL;
	}
	
	return;
}

static int dec_fill_bsbuffer(DecHandle handle, 	u32 bs_va_startaddr, u32 bs_va_endaddr,
	u32 bs_pa_startaddr, T_DATA_PACKET *ptPkt)
{
	RetCode ret;
	PhysicalAddress pa_read_ptr, pa_write_ptr;
	u32 target_addr, space;
	int size = STREAM_END_SIZE;
	int room;

    if(NULL == ptPkt)
    {
        ret = vpu_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);
	    if (ret != RETCODE_SUCCESS) {
		    err_msg("vpu_DecUpdateBitstreamBuffer failed=%d\n",ret);
		    return -1;
	    }
        return 0;
    }
    
    size = ptPkt->iLen;  
	ret = vpu_DecGetBitstreamBuffer(handle, &pa_read_ptr, &pa_write_ptr,
		&space);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecGetBitstreamBuffer failed\n");
		return -1;
	}

	/* Decoder bitstream buffer is full */
	if (space <= 0 ||space <size ) {
		warn_msg("space %u <size =%d\n", space,size);
		return 0;
	}
	
	/* Fill the bitstream buffer */
	target_addr = bs_va_startaddr + (pa_write_ptr - bs_pa_startaddr);

    if(target_addr < bs_va_startaddr)
    {
        err_msg("target_addr:%u < bs_va_startaddr:%u\n",
            target_addr,bs_va_startaddr);
    }
    
	if ( (target_addr + size) > bs_va_endaddr) {
		room = bs_va_endaddr - target_addr;
		memcpy((void *)target_addr,ptPkt->pcData,room);
		memcpy((void *)bs_va_startaddr,(void *)(ptPkt->pcData + room),size - room);

	} else {
		memcpy((void *)target_addr,ptPkt->pcData,size);
	}
	ret = vpu_DecUpdateBitstreamBuffer(handle, size);
	if (ret != RETCODE_SUCCESS) {
		err_msg("vpu_DecUpdateBitstreamBuffer failed=%d\n",ret);
		return -1;
	}
	return size;
}

int VPU_DecClrDispFlagCallback(void *arg, int iDisplayIndex)
{
    int iRet = 0;
    T_VDEC_INFO *ptVpuInfo = (T_VDEC_INFO*)arg;
    T_DATA_PACKET tPkt;

    if (NULL == ptVpuInfo || iDisplayIndex <0 || iDisplayIndex>= ptVpuInfo->iRegfbcount)
    {
         return -1;
    }

    tPkt.pcData = NULL;
    tPkt.iLen = iDisplayIndex;  
    packet_queue_put(&ptVpuInfo->tPackIpu, &tPkt);   

    #if 0
    iRet = vpu_DecClrDispFlag((DecHandle)(ptVpuInfo->hDecHandle), iDisplayIndex);
    if (iRet)
    {
        err_msg("vpu_DecClrDispFlag failed Error code, %d\n", iRet);
        return -1;
    }
    
    
	printf("vpu_DecClrDispFlag succ iDisplayIndex:%d pHandle:%p\n", iDisplayIndex,ptVpuInfo);

    #endif
    return 0;
}

void *vpu_vdec_loop(void *arg)
{
	T_VDEC_INFO *ptVpuInfo = (T_VDEC_INFO*)arg;
	if(NULL == ptVpuInfo)
	{
		return NULL;
	}
	
	DecHandle handle = ptVpuInfo->hDecHandle;
	DecOutputInfo outinfo = {0};
	DecParam decparam = {0};
	RetCode eRetCode = RETCODE_FAILURE;
	int loop_id = 0;
	double  frame_id = 0 ;
	int totalNumofErrMbs = 0;
	int actual_display_index = -1;
	int is_waited_int = 0;
	T_DATA_PACKET tPkt;
	int iHaveParseAndAlloc = 0;   //0未解析 1已解析未分配
	decparam.dispReorderBuf = 0;
	decparam.skipframeMode = 0;
	decparam.skipframeNum = 0;
	struct frame_buf *pfb = NULL;
	int iPacketCount = 0;
    struct timeval tpstart,tpend;
    double timeuse;

    ptVpuInfo->iHaveParseAndAlloc = 0;
    info_msg("enter ptVpuInfo:%p\n",ptVpuInfo);
    //int iFirst = 0; 

	/*
	* once iframeSearchEnable is enabled, prescanEnable, prescanMode
	* and skipframeMode options are ignored.
	*/
	decparam.iframeSearchEnable = 0;
	while(!ptVpuInfo->iTaskExitFlag )
    {   

        //测试下解码第一帧的时间
        /*if(0== iFirst)
        {
            iPacketCount = 0;
            gettimeofday(&tpstart,NULL);
            iFirst = 1;
        }*/

        int iPktNum = packet_queue_get_packet_num(&ptVpuInfo->tPacketQueue);
        int iRet = 0;

        //info_msg("decstate =%d\n", ptVpuInfo->iDecState);

        memset(&tPkt,0,sizeof(tPkt));
        
        if(E_NO_DEC == ptVpuInfo->iDecState)
        {
            if(iPktNum >50)
            {
                packet_queue_get(&ptVpuInfo->tPacketQueue, &tPkt, 0);
                free(tPkt.pcData);
			    tPkt.pcData = NULL; 
            }
            usleep(5000);
            continue;
       }
        else if (E_DEC_START == ptVpuInfo->iDecState)
        {
            iRet = packet_queue_get_first_IFrame(&ptVpuInfo->tPacketQueue, &tPkt);
            if( 0 == iRet )
            {
                 ptVpuInfo->iDecState = E_DEC_DISP;
                 continue;
            }
            else
            {
                ptVpuInfo->iDecState = E_DEC_STARTING;
                usleep(5000);
            }
        }
        else if (E_DEC_STARTING == ptVpuInfo->iDecState)
        {
            PT_DATA_PACKET_LIST ptList = NULL;
            
            ptList = FindIFrameFromeQueue(&ptVpuInfo->tPacketQueue);
            
            if (ptList)
            {
                iRet = packet_queue_get_first_IFrame(&ptVpuInfo->tPacketQueue, &tPkt);
                ptVpuInfo->iDecState = E_DEC_DISP;
                usleep(25000);
            }
            else
            {
                 iRet = packet_queue_get(&ptVpuInfo->tPacketQueue, &tPkt, 0);
                 if (0 == iRet)
                 {
                     ptVpuInfo->iDecState = E_DEC_DISP;
                     usleep(5000);
                     continue;
                 }
                 else
                 {
                    usleep(25000);
                 }
            }
        }
        else
        {
            // normal
            iRet = packet_queue_get(&ptVpuInfo->tPacketQueue, &tPkt, 0);
            if (0 == iRet )
            {
                usleep(5000);
                if (iPacketCount == 3000) {
                    info_msg("packet_queue_get fail iConut=%d\n",iPacketCount);
                    iPacketCount = 0;
                }
                iPacketCount ++;
                continue;
            }
        }
		
		iPacketCount =0;
		iRet  = dec_fill_bsbuffer(ptVpuInfo->hDecHandle,ptVpuInfo->mem_desc.virt_uaddr,ptVpuInfo->mem_desc.virt_uaddr + STREAM_BUF_SIZE	,
			ptVpuInfo->mem_desc.phy_addr,&tPkt);

        free(tPkt.pcData);
		tPkt.pcData = NULL; 
		if (iRet < 0) {
			err_msg("dec_fill_bsbuffer failed\n");
			
			continue;
		}	
		if(0 == iHaveParseAndAlloc )
		{				
		    pthread_mutex_lock( &g_hAllocMutex );
		    info_msg("begin decoder_parse:%p\n",ptVpuInfo); 
			iRet = decoder_parse(ptVpuInfo);
			if (iRet) {
				//err_msg("decoder parse failed\n");
				info_msg("decoder parse failed:%p\n",ptVpuInfo);
                pthread_mutex_unlock( &g_hAllocMutex );
				continue;
			}
			info_msg("begin IOGetPhyMem:%p\n",ptVpuInfo);	
			iRet = IOGetPhyMem(&ptVpuInfo->slice_mem_desc);
			if (iRet) {
				err_msg("Unable to obtain physical slice save mem\n");
                pthread_mutex_unlock( &g_hAllocMutex );
				break;
			}
            //info_msg("begin decoder_allocate_framebuffer:%p\n",ptVpuInfo);
			iRet = decoder_allocate_framebuffer(ptVpuInfo);
			if (iRet)
			{
				err_msg("decoder_allocate_framebuffer err\n");
                IOFreePhyMem(&ptVpuInfo->slice_mem_desc);
                pthread_mutex_unlock( &g_hAllocMutex );
				break;
			}
            info_msg("end_allocate_framebuffer:%p\n",ptVpuInfo);
            pthread_mutex_unlock( &g_hAllocMutex );
			iHaveParseAndAlloc = 1;
            ptVpuInfo->iHaveParseAndAlloc = 1;
            
		}
	
		/*
		* For mx6x MJPG decoding with streaming mode
		* bitstream buffer filling cannot be done when JPU is in decoding,
		* there are three places can do this:
		* 1. before vpu_DecStartOneFrame;
		* 2. in the case of RETCODE_JPEG_BIT_EMPTY returned in DecStartOneFrame() func;
		* 3. after vpu_DecGetOutputInfo.
		*/

		while(1)
		{	    
		    T_DATA_PACKET tIpuPkt;
            int iret = 0;
            iret = packet_queue_get(&ptVpuInfo->tPackIpu, &tIpuPkt, 0);
            while(iret)
            {
                int iDisIndex = tIpuPkt.iLen;

                eRetCode = vpu_DecClrDispFlag(handle,iDisIndex);
			    if (eRetCode)
				    err_msg("vpu_DecClrDispFlag failed Error code"
				    " %d\n", eRetCode);
                iret = packet_queue_get(&ptVpuInfo->tPackIpu, &tIpuPkt, 0);
            }
            
		    eRetCode = vpu_DecStartOneFrame(handle, &decparam);      

			if (eRetCode == RETCODE_JPEG_EOS) {
				info_msg(" JPEG bitstream is end\n");
				break;
			} else if (eRetCode == RETCODE_JPEG_BIT_EMPTY) {
				break;
			}
			if (eRetCode != RETCODE_SUCCESS) {
				err_msg("DecStartOneFrame failed, ret=%d\n", eRetCode);
				ptVpuInfo->iTaskExitFlag = 1;
				break;
			}	

            /*if(1 == iFirst)
                {
                    gettimeofday(&tpend,NULL);   
            timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;//娉ㄦ剰锛岀鐨勮鏁板拰寰鐨勮鏁伴兘搴旇绠楀湪鍐?
            printf("used time:%fus\n",timeuse); 
            iFirst = 2;
                }*/
			is_waited_int = 0;
			loop_id = 0;
			while (vpu_IsBusy()) {

				if (loop_id == 50) {
					info_msg(" vpu_SWReset\n");
                    vpu_SWReset(handle, 0);
					break;
				}
				vpu_WaitForInt(50);
				is_waited_int = 1;
				loop_id ++;
			}
			if (!is_waited_int)
				vpu_WaitForInt(50);
			eRetCode = vpu_DecGetOutputInfo(handle, &outinfo);
            
			if (eRetCode != RETCODE_SUCCESS) {
				err_msg("vpu_DecGetOutputInfo failed Err code is %d\n"
					"\tframe_id = %d\n", eRetCode, (int)frame_id);
				ptVpuInfo->iTaskExitFlag = 1;
				break;
			}

			if (outinfo.decodingSuccess == 0) {
				warn_msg("Incomplete finish of decoding process.\n"
					"\tframe_id = %d\n", (int)frame_id);
				break;
			}
			if (cpu_is_mx6x() && (outinfo.decodingSuccess & 0x10)) {
				/*warn_msg("vpu needs more bitstream in rollback mode\n"
					"\tframe_id = %d\n", (int)frame_id);	*/				
				break;
			}

			if (outinfo.notSufficientPsBuffer) {
				err_msg("PS Buffer overflow\n");
				break;
			}
			if (outinfo.notSufficientSliceBuffer) {
				err_msg("Slice Buffer overflow\n");
				break;
			}
			
			/* BIT don't have picture to be displayed */
			if(outinfo.indexFrameDisplay <0 ||outinfo.indexFrameDisplay >=ptVpuInfo->iRegfbcount ) {
				//err_msg("VPU doesn't have picture to be displayed.\n"
				//	"\toutinfo.indexFrameDisplay = %d\n",outinfo.indexFrameDisplay);
				break;
			}
			actual_display_index = outinfo.indexFrameDisplay;
 #if 0
			eRetCode = vpu_DecClrDispFlag(handle,actual_display_index);
			if (eRetCode)
				err_msg("vpu_DecClrDispFlag failed Error code"
				" %d\n", eRetCode);
#endif
			//info_msg("display_index = %d\n",outinfo.indexFrameDisplay);
			
			pfb = ptVpuInfo->pfbpool[actual_display_index];
			/* User Data */
			if (outinfo.indexFrameDecoded >= 0 &&
				outinfo.userData.enable && outinfo.userData.size) {
					if (outinfo.userData.userDataBufFull)
						warn_msg("User Data Buffer is Full\n");
			}

			if(outinfo.indexFrameDecoded >= 0) {
				/* We MUST be careful of sequence param change (resolution change, etc)
				* Different frame buffer number or resolution may require vpu_DecClose
				* and vpu_DecOpen again to reallocate sufficient resources.
				* If you already allocate enough frame buffers of max resolution
				* in the beginning, you may not need vpu_DecClose, etc. But sequence
				* headers must be ahead of their pictures to signal param change.
				*/
				if ((outinfo.decPicWidth != ptVpuInfo->iLastPicWidth)
					||(outinfo.decPicHeight != ptVpuInfo->iLastPicHeight)) {
						//warn_msg("resolution changed from %dx%d to %dx%d\n",
						//	ptVpuInfo->iLastPicWidth, ptVpuInfo->iLastPicHeight,
						//	outinfo.decPicWidth, outinfo.decPicHeight);

						ptVpuInfo->iLastPicWidth = outinfo.decPicWidth;
						ptVpuInfo->iLastPicHeight = outinfo.decPicHeight;
						ptVpuInfo->iPicWidth = outinfo.decPicWidth;
						ptVpuInfo->iPicHeight = outinfo.decPicHeight;
						if(!ptVpuInfo->hIpuHandel)
						{
							ptVpuInfo->hIpuHandel = ipu_createtask(outinfo.decPicWidth,outinfo.decPicHeight,&ptVpuInfo->tOutRect,VPU_DecClrDispFlagCallback,ptVpuInfo);
                            //ptVpuInfo->hIpuHandel = ipu_createtask(outinfo.decPicWidth,outinfo.decPicHeight,&ptVpuInfo->tOutRect,NULL,NULL);
							if(ptVpuInfo->iDispEnable)
							{
								ipu_enableDisp(ptVpuInfo->hIpuHandel);
							}
						}
						else
						{	
							ipu_destroytask(ptVpuInfo->hIpuHandel);
							ptVpuInfo->hIpuHandel = 0;
							ptVpuInfo->iTaskExitFlag = 1;
							info_msg(" resolution changed\n");
							break;
						}
				}
				if (outinfo.numOfErrMBs) {
					totalNumofErrMbs += outinfo.numOfErrMBs;
					info_msg("Num of Error Mbs : %d\n",
						outinfo.numOfErrMBs);
				}
			}
			if(ptVpuInfo->hIpuHandel )
			{
				ipu_senddisaddr(ptVpuInfo->hIpuHandel,pfb->desc.phy_addr,pfb->desc.virt_uaddr,actual_display_index);
			}
			
			frame_id++;
		}
	}

    if(iHaveParseAndAlloc)
    {
        dec_fill_bsbuffer(ptVpuInfo->hDecHandle,ptVpuInfo->mem_desc.virt_uaddr,ptVpuInfo->mem_desc.virt_uaddr + STREAM_BUF_SIZE    ,
               ptVpuInfo->mem_desc.phy_addr,NULL);
    }
    
    info_msg("leave1 ptVpuInfo:%p\n",ptVpuInfo);
    if(ptVpuInfo->hIpuHandel)
	{
		ipu_destroytask(ptVpuInfo->hIpuHandel);
		ptVpuInfo->hIpuHandel = NULL;		
	}
    info_msg("leave2 ptVpuInfo:%p\n",ptVpuInfo);

    usleep(10*1000);
    decoder_close(ptVpuInfo);
    usleep(10*1000);
	return NULL;
}
VDEC_HADNDLE VDEC_CreateVideoDecCh()
{
	T_VDEC_INFO* ptVpuInfo = NULL;
	vpu_mem_desc *pMem_desc = NULL;
	vpu_mem_desc *pPs_mem_desc = NULL;
	int ret;
	ptVpuInfo = (T_VDEC_INFO*)malloc(sizeof(T_VDEC_INFO));
	if (NULL == ptVpuInfo)
	{
		info_msg("Failed to allocate T_VPU_INFO\n");
		return 0;
	}
	memset(ptVpuInfo,0,sizeof(T_VDEC_INFO));

    ptVpuInfo->iDecState = E_NO_DEC;
	pMem_desc = &ptVpuInfo->mem_desc;
	pPs_mem_desc = &ptVpuInfo->ps_mem_desc;
	pMem_desc->size = STREAM_BUF_SIZE;
	//before opening the instance to input oparam.bitstreamBuffer
	ret = IOGetPhyMem(pMem_desc);
	if (ret) {
		info_msg("Unable to obtain physical mem\n");
		goto err;
	}
	//to get the corresponding virtual address of the bitstream buffer
	if (IOGetVirtMem(pMem_desc) <= 0) {
		info_msg("Unable to obtain virtual mem\n");
		ret = -1;
		goto err0;
	}
	//for both the physical PS save buffer and physical	slice save memory for H.264.
	pPs_mem_desc->size = PS_SAVE_SIZE;
	ret = IOGetPhyMem(pPs_mem_desc);
	if (ret) {
		info_msg("Unable to obtain physical ps save mem\n");
		goto err1;
	}
	//Open a decoder instance 
	ret = decoder_open(ptVpuInfo);	
	if (ret)
	{
		info_msg("decoder_open err\n");
		goto err2;
	}
	packet_queue_init(&ptVpuInfo->tPacketQueue);
    
	packet_queue_init(&ptVpuInfo->tPackIpu);
    
	return  (VDEC_HADNDLE)ptVpuInfo;

err2:
	IOFreePhyMem(pPs_mem_desc);
err1:
	IOFreeVirtMem(pMem_desc);
err0:
	IOFreePhyMem(pMem_desc);
err:
	if (ptVpuInfo)
		free(ptVpuInfo);
	return 0;
}

int VDEC_DestroyVideoDecCh(VDEC_HADNDLE VHandle)
{
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;
	if(NULL == ptVpuInfo)
	{
		return -1;
	}
	ptVpuInfo->iTaskExitFlag = 1;
	packet_queue_uninit(&ptVpuInfo->tPacketQueue);
    packet_queue_uninit(&ptVpuInfo->tPackIpu);
	//decoder_close(ptVpuInfo);
	/* free the frame buffers */

    if(ptVpuInfo->iHaveParseAndAlloc)
	{
		decoder_free_framebuffer(ptVpuInfo);
		IOFreePhyMem(&ptVpuInfo->slice_mem_desc);
	}

	IOFreePhyMem(&ptVpuInfo->ps_mem_desc);
	IOFreeVirtMem(&ptVpuInfo->mem_desc);
	IOFreePhyMem(&ptVpuInfo->mem_desc);
    usleep(10*1000);
	free(ptVpuInfo);
	ptVpuInfo = NULL;
	return 0;
}

int VDEC_SetDecWindowsPos(VDEC_HADNDLE VHandle, int x, int y, int iWidth, int iHeight)
{
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;
	if(NULL == ptVpuInfo)
	{
		return -1;
	}
	ptVpuInfo->tOutRect.iLeft = x;
	ptVpuInfo->tOutRect.iTop  = y;
	ptVpuInfo->tOutRect.iWidth = iWidth;
	ptVpuInfo->tOutRect.iHeight = iHeight;
	if(ptVpuInfo->hIpuHandel)
	{
		ipu_changePos(ptVpuInfo->hIpuHandel,&ptVpuInfo->tOutRect);
	}
	return 0;
}


int VDEC_StartVideoDec(VDEC_HADNDLE VHandle)
{
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;
	if(NULL == ptVpuInfo)
	{
		return -1;
	}
 //   info_msg(" begin\n");
	ptVpuInfo->iTaskExitFlag = 0;
	pthread_create(&ptVpuInfo->pthreadId,NULL, vpu_vdec_loop,(void *)ptVpuInfo);
  //  info_msg(" end\n");
    return 0;
}

int VDEC_StopVideoDec(VDEC_HADNDLE VHandle)
{
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;
	if(NULL == ptVpuInfo)
	{
		return -1;
	}
    info_msg(" begin\n");
	ptVpuInfo->iTaskExitFlag = 1;
	ptVpuInfo->iDispEnable = 0;
	ipu_disableDisp(ptVpuInfo->hIpuHandel);
	pthread_join(ptVpuInfo->pthreadId, NULL);
	ptVpuInfo->pthreadId = 0;
    info_msg(" end\n");
	return 0;
}

static bool IsIFrame(const unsigned char *pv,int len)
{
	int i=0;
	len = len-4;
	int minlen = ((len < 300) ? len : 300);

	int sps = 0, pps =0;
	for (i=0;i<minlen;++i)
	{
		if (pv[i]==0 && pv[i+1]==0 && pv[i+2]==0 && pv[i+3]==1 && ((pv[i+4] & 0x1F)==0x07))
		{
			sps=1;
			break;
		}
	}
	for (i=0;i<minlen;++i)
	{
		if (pv[i]==0 && pv[i+1]==0 && pv[i+2]==0 && pv[i+3]==1 && ((pv[i+4] & 0x1F)==0x08))
		{
			pps=1;
			break;
		}
	}
	if (sps && pps)
		return 1;
	else
		return 0;
}


int VDEC_SendVideoStream(VDEC_HADNDLE VHandle, char *pcFrame, int iLen)
{ 
    T_DATA_PACKET tPkt;
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;
	if(NULL == ptVpuInfo || iLen <1)
	{
		return -1;
	}
	if( !ptVpuInfo->iTaskExitFlag)
	{
        tPkt.iIFrameFlag = 0;
    
        //不显示的时候就不解码，所以加上I帧标志，方便跳帧
        if((E_NO_DEC == ptVpuInfo->iDecState) || (E_DEC_START == ptVpuInfo->iDecState)
            || (E_DEC_STARTING == ptVpuInfo->iDecState))
        {
            tPkt.iIFrameFlag = IsIFrame(pcFrame,iLen);
        } 
       
        tPkt.pcData = (char *)malloc(iLen +32);
        tPkt.iLen = iLen;
        memcpy(tPkt.pcData, pcFrame, iLen);
        packet_queue_put(&ptVpuInfo->tPacketQueue, &tPkt);   
       }
    return 0;
}



int VDEC_VideoDecDisp(VDEC_HADNDLE VHandle,int iDispEnable,int iDecEnable)
{
    T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;

	if(NULL == ptVpuInfo)
	{
		return -1;
	}
    
    ptVpuInfo->iDispEnable = iDispEnable?1:0;
    ptVpuInfo->iDecState = iDecEnable?E_DEC_START:E_NO_DEC;

    info_msg(" iDispEnable:%d\n",iDispEnable)
    
    if(iDispEnable)
    {
        ipu_enableDisp(ptVpuInfo->hIpuHandel);
    }
    else
    {
        ipu_disableDisp(ptVpuInfo->hIpuHandel);
    }
    return 0;
}

int VDEC_GetVideoDecDispEnable(VDEC_HADNDLE VHandle,int *piVdecEnable,int *piDispEnable)
{
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;

	if(NULL == ptVpuInfo)
	{
		return -1;
	}
    *piVdecEnable = (ptVpuInfo->iDecState != E_NO_DEC)?1:0;
    *piDispEnable = ptVpuInfo->iDispEnable;
    
	return 0 ;
}

int VDEC_Setfb1BKColor(VDEC_HADNDLE VHandle,int x,int y,int w,int h)
{
	T_VDEC_INFO* ptVpuInfo = (T_VDEC_INFO*)VHandle;
	if(ptVpuInfo)
	{
		if(ptVpuInfo->iDispEnable)
		{
			return ipu_setfb1Bkcolor(ptVpuInfo->hIpuHandel,x, y, w, h);
		}
		return 0;
	}
	else
	{
		return ipu_setfb1Bkcolor(NULL,x, y, w, h);
	}
}

