/*
 * Copyright 2004-2012 Freescale Semiconductor, Inc.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fb.h"
#include "commondef.h"


typedef struct _T_FB_PACKET
{
	int iWidth;
    int iHeight;
    struct frame_buf *pFb;
}T_FB_PACKET, *PT_FB_PACKET;

typedef struct _T_FB_PACKET_LIST
{
	T_FB_PACKET tPkt;
	struct _T_FB_PACKET_LIST *next;
}T_FB_PACKET_LIST, *PT_FB_PACKET_LIST;

typedef struct _T_FB_PACKET_QUEUE {
	T_FB_PACKET_LIST *first_pkt, *last_pkt;
	int nb_packets;
	pthread_mutex_t hMutex;
    
}T_FB_PACKET_QUEUE;


#define NUM_FRAME_BUFS	64
#define FB_INDEX_MASK	(NUM_FRAME_BUFS - 1)

static int fb_index;
static struct frame_buf *fbarray[NUM_FRAME_BUFS];
static struct frame_buf fbpool[NUM_FRAME_BUFS];

static pthread_mutex_t g_tFbMutex;
static T_FB_PACKET_QUEUE *g_ptFbQueue;


void Fb_packet_queue_flush(T_FB_PACKET_QUEUE *q)
{
	PT_FB_PACKET_LIST pkt, pkt1;
	if(NULL ==q )
	{
		return;
	}
    
#ifdef _WINDOWS_
	WaitForSingleObject( q->hMutex, INFINITE );
#else
	pthread_mutex_lock( &q->hMutex );
#endif
	for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		if((NULL!=pkt) )
		{  
			freeFreameBuf(pkt->tPkt.pFb);
			free(pkt);
		}
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif

}

int Fb_Put_Packet(T_FB_PACKET_QUEUE *q,PT_FB_PACKET pkt)
{
    PT_FB_PACKET_LIST pktl = NULL;

    if (NULL == pkt || NULL == q)
		return -1;

	pktl = (PT_FB_PACKET_LIST)malloc(sizeof(T_FB_PACKET_LIST));
	if (!pktl)
		return -1;
	pktl->tPkt = *pkt;
	pktl->next = NULL;

#ifdef _WINDOWS_
        WaitForSingleObject( q->hMutex, INFINITE );
#else
        pthread_mutex_lock( &q->hMutex );
#endif
	
	if (!q->last_pkt)

		q->first_pkt = pktl;
	else
		q->last_pkt->next = pktl;
	q->last_pkt = pktl;
	q->nb_packets++;

#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif
    
	return 0;
}

struct frame_buf * Fb_Get_packet(T_FB_PACKET_QUEUE *q,int iWidth,int iHeight)
{
   PT_FB_PACKET_LIST ptList = NULL;
   PT_FB_PACKET_LIST ptListTmp = NULL;
   struct frame_buf *pFindFrame = NULL;
   
   if(NULL == q)
   {
        return NULL;
   }

#ifdef _WINDOWS_
	WaitForSingleObject( q->hMutex, INFINITE );
#else
	pthread_mutex_lock( &q->hMutex );
#endif

    ptList = q->first_pkt;
    while (ptList)
    {
        
         if ((ptList->tPkt.iHeight == iHeight) && (ptList->tPkt.iWidth == iWidth))
         {
             pFindFrame = ptList->tPkt.pFb;
             if(NULL == ptListTmp)
             {
                q->first_pkt = ptList->next;
             }
             else
             {
                ptListTmp->next = ptList->next;
             }
             
             if (q->last_pkt == ptList)
             {
                q->last_pkt = ptListTmp;
             }
                
             if(!q->first_pkt)
             {
                q->last_pkt = NULL;
             }
             free(ptList);
             q->nb_packets--;
             break;
         }
         ptListTmp = ptList;
         ptList = ptList->next;
    }
#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif

    
    return pFindFrame;
}

int Fb_get_packet_num(T_FB_PACKET_QUEUE *q)
{
    int iRet =0;
	if(NULL==q)
	{
		return 0;
	}
#ifdef _WINDOWS_
	WaitForSingleObject( q->hMutex, INFINITE );
#else
	pthread_mutex_lock( &q->hMutex );
#endif
    iRet = q->nb_packets;

#ifdef _WINDOWS_
        ReleaseMutex( q->hMutex);
#else
        pthread_mutex_unlock( &q->hMutex );
#endif
    return iRet;
   
}


void framebuf_init(void)
{
	int i;
	fb_index = 0;
	for (i = 0; i < NUM_FRAME_BUFS; i++) {
		fbarray[i] = &fbpool[i];
	}
	pthread_mutex_init(&g_tFbMutex, NULL);  
    g_ptFbQueue =(T_FB_PACKET_QUEUE*) malloc(sizeof(T_FB_PACKET_QUEUE));
    memset(g_ptFbQueue, 0, sizeof(T_FB_PACKET_QUEUE));
    pthread_mutex_init(&g_ptFbQueue->hMutex,NULL);  
}


struct frame_buf *get_framebuf(void)
{
	struct frame_buf *fb;
		
	pthread_mutex_lock( &g_tFbMutex);
    if(fb_index >=NUM_FRAME_BUFS)
    {
        printf("get_framebuf err fb_index:%d\n",fb_index);
        pthread_mutex_unlock(&g_tFbMutex);
        return 0;
    }
	fb = fbarray[fb_index];
	fbarray[fb_index] = 0;

	++fb_index;
	//fb_index &= FB_INDEX_MASK;
	pthread_mutex_unlock(&g_tFbMutex);

   
	return fb;
}

void put_framebuf(struct frame_buf *fb)
{
	pthread_mutex_lock( &g_tFbMutex);
	
	//fb_index &= FB_INDEX_MASK;
	if(fb_index <=0)
    {
        printf("put_framebuf err fb_index:%d\n",fb_index);
        pthread_mutex_unlock(&g_tFbMutex);
        return ;
    }
	--fb_index;
	fbarray[fb_index] = fb;
	pthread_mutex_unlock(&g_tFbMutex);
}

struct frame_buf *framebuf_alloc(int stdMode, int format, int strideY, int height, int mvCol)
{
	struct frame_buf *fb;
	int err;
	int divX, divY;

    fb = Fb_Get_packet(g_ptFbQueue, strideY, height);
    if(fb)
    {
        return fb;
    }
	//fb = get_framebuf();
	//if (fb == NULL)
	{
	    if(Fb_get_packet_num(g_ptFbQueue)>0)
        {
            Fb_packet_queue_flush(g_ptFbQueue);
        }   
        fb = get_framebuf();
        if (fb == NULL)
        {
            err_msg("get_framebuf failure\n");
		    return NULL;
        }
	}

    //info_msg("strideY:%d height:%d\n",strideY,height);
	divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	divY = (format == MODE420 || format == MODE224) ? 2 : 1;

	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
	fb->desc.size = (strideY * height  + strideY / divX * height / divY * 2)+512;
	if (mvCol)
		fb->desc.size += strideY / divX * height / divY;

	err = IOGetPhyMem(&fb->desc);
	if (err) {
		err_msg("Frame buffer allocation failure\n");
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	fb->addrY = fb->desc.phy_addr;
	fb->addrCb = fb->addrY + strideY * height;
	fb->addrCr = fb->addrCb + strideY / divX * height / divY;
	fb->strideY = strideY;
	fb->strideC =  strideY / divX;
	if (mvCol)
		fb->mvColBuf = fb->addrCr + strideY / divX * height / divY+512;

	if (IOGetVirtMem(&(fb->desc)) <= 0) {
		IOFreePhyMem(&fb->desc);
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		err_msg("IOGetVirtMem failure\n");
		return NULL;
	}

	return fb;
}

int tiled_framebuf_base(FrameBuffer *fb, Uint32 frame_base, int strideY, int height, int mapType)
{
	int align;
	int divX, divY;
	Uint32 lum_top_base, lum_bot_base, chr_top_base, chr_bot_base;
	Uint32 lum_top_20bits, lum_bot_20bits, chr_top_20bits, chr_bot_20bits;
	int luma_top_size, luma_bot_size, chroma_top_size, chroma_bot_size;

	divX = 2;
	divY = 2;

	/*
	 * The buffers is luma top, chroma top, luma bottom and chroma bottom for
	 * tiled map type, and only 20bits for the address description, so we need
	 * to do 4K page align for each buffer.
	 */
	align = SZ_4K;
	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		/* luma_top_size means the Y size of one frame, chroma_top_size
		 * means the interleaved UV size of one frame in frame tiled map type*/
		luma_top_size = (strideY * height + align - 1) & ~(align - 1);
		chroma_top_size = (strideY / divX * height / divY * 2 + align - 1) & ~(align - 1);
		luma_bot_size = chroma_bot_size = 0;
	} else {
		/* This is FIELD_FRAME_MB_RASTER_MAP case, there are two fields */
		luma_top_size = (strideY * height / 2 + align - 1) & ~(align - 1);
		luma_bot_size = luma_top_size;
		chroma_top_size = (strideY / divX * height / divY + align - 1) & ~(align - 1);
		chroma_bot_size = chroma_top_size;
	}

	lum_top_base = (frame_base + align - 1) & ~(align -1);
	chr_top_base = lum_top_base + luma_top_size;
	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		lum_bot_base = 0;
		chr_bot_base = 0;
	} else {
		lum_bot_base = chr_top_base + chroma_top_size;
		chr_bot_base = lum_bot_base + luma_bot_size;
	}

	lum_top_20bits = lum_top_base >> 12;
	lum_bot_20bits = lum_bot_base >> 12;
	chr_top_20bits = chr_top_base >> 12;
	chr_bot_20bits = chr_bot_base >> 12;

	/*
	 * In tiled map format the construction of the buffer pointers is as follows:
	 * 20bit = addrY [31:12]: lum_top_20bits
	 * 20bit = addrY [11: 0], addrCb[31:24]: chr_top_20bits
	 * 20bit = addrCb[23: 4]: lum_bot_20bits
	 * 20bit = addrCb[ 3: 0], addrCr[31:16]: chr_bot_20bits
	 */
	fb->bufY = (lum_top_20bits << 12) + (chr_top_20bits >> 8);
	fb->bufCb = (chr_top_20bits << 24) + (lum_bot_20bits << 4) + (chr_bot_20bits >> 16);
	fb->bufCr = chr_bot_20bits << 16;

    return 0;
}

struct frame_buf *tiled_framebuf_alloc(int stdMode, int format, int strideY, int height, int mvCol, int mapType)
{
	struct frame_buf *fb;
	int err, align;
	int divX, divY;
	Uint32 lum_top_base, lum_bot_base, chr_top_base, chr_bot_base;
	Uint32 lum_top_20bits, lum_bot_20bits, chr_top_20bits, chr_bot_20bits;
	int luma_top_size, luma_bot_size, chroma_top_size, chroma_bot_size;

	fb = get_framebuf();
	if (fb == NULL)
		return NULL;

	divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	divY = (format == MODE420 || format == MODE224) ? 2 : 1;

	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));

	/*
	 * The buffers is luma top, chroma top, luma bottom and chroma bottom for
	 * tiled map type, and only 20bits for the address description, so we need
	 * to do 4K page align for each buffer.
	 */
	align = SZ_4K;
	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		/* luma_top_size means the Y size of one frame, chroma_top_size
		 * means the interleaved UV size of one frame in frame tiled map type*/
		luma_top_size = (strideY * height + align - 1) & ~(align - 1);
		chroma_top_size = (strideY / divX * height / divY * 2 + align - 1) & ~(align - 1);
		luma_bot_size = chroma_bot_size = 0;
	} else {
		/* This is FIELD_FRAME_MB_RASTER_MAP case, there are two fields */
		luma_top_size = (strideY * height / 2 + align - 1) & ~(align - 1);
		luma_bot_size = luma_top_size;
		chroma_top_size = (strideY / divX * height / divY + align - 1) & ~(align - 1);
		chroma_bot_size = chroma_top_size;
	}
	fb->desc.size = luma_top_size + chroma_top_size + luma_bot_size + chroma_bot_size;
	/* There is possible fb->desc.phy_addr in IOGetPhyMem not 4K page align,
	 * so add more SZ_4K byte here for alignment */
	fb->desc.size += align - 1;

	if (mvCol)
		fb->desc.size += strideY / divX * height / divY;

	err = IOGetPhyMem(&fb->desc);
	if (err) {
		err_msg("Frame buffer allocation failure\n");
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	if (IOGetVirtMem(&(fb->desc)) <= 0) {
		IOFreePhyMem(&fb->desc);
		memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
		return NULL;
	}

	lum_top_base = (fb->desc.phy_addr + align - 1) & ~(align -1);
	chr_top_base = lum_top_base + luma_top_size;
	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		lum_bot_base = 0;
		chr_bot_base = 0;
	} else {
		lum_bot_base = chr_top_base + chroma_top_size;
		chr_bot_base = lum_bot_base + luma_bot_size;
	}

	lum_top_20bits = lum_top_base >> 12;
	lum_bot_20bits = lum_bot_base >> 12;
	chr_top_20bits = chr_top_base >> 12;
	chr_bot_20bits = chr_bot_base >> 12;

	/*
	 * In tiled map format the construction of the buffer pointers is as follows:
	 * 20bit = addrY [31:12]: lum_top_20bits
	 * 20bit = addrY [11: 0], addrCb[31:24]: chr_top_20bits
	 * 20bit = addrCb[23: 4]: lum_bot_20bits
	 * 20bit = addrCb[ 3: 0], addrCr[31:16]: chr_bot_20bits
	 */
	fb->addrY = (lum_top_20bits << 12) + (chr_top_20bits >> 8);
	fb->addrCb = (chr_top_20bits << 24) + (lum_bot_20bits << 4) + (chr_bot_20bits >> 16);
	fb->addrCr = chr_bot_20bits << 16;
	fb->strideY = strideY;
	fb->strideC = strideY / divX;
	if (mvCol) {
		if (mapType == TILED_FRAME_MB_RASTER_MAP) {
			fb->mvColBuf = chr_top_base + chroma_top_size;
		} else {
			fb->mvColBuf = chr_bot_base + chroma_bot_size;
		}
	}

    return fb;
}

void framebuf_free(struct frame_buf *fb,int iWidth,int iHeight)
{
    T_FB_PACKET tPkt;

    if (fb == NULL)
     return;
    tPkt.iHeight = iHeight;
    tPkt.iWidth = iWidth;
    tPkt.pFb = fb;
	Fb_Put_Packet(g_ptFbQueue, &tPkt);
}

void freeFreameBuf(struct frame_buf *fb)
{
    if (fb == NULL)
            return;
    
    if (fb->desc.virt_uaddr) {
         IOFreeVirtMem(&fb->desc);
    }
    
    if (fb->desc.phy_addr) {
        IOFreePhyMem(&fb->desc);
    }
    
    memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
    put_framebuf(fb);
}

