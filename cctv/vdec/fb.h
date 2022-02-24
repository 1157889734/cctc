#ifndef  __FB_H__
#define  __FB_H__

#include <linux/videodev2.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <semaphore.h>
#include "mxc_ipu_hl_lib.h"
#include "vpu_lib.h"
#include "vpu_io.h"

#define SZ_4K			(4 * 1024)


#define STREAM_BUF_SIZE		0x200000
#define PS_SAVE_SIZE		0x080000
#define SLICE_SAVE_SIZE     3264512      //参考几个解码例子得的 具体怎么来的不知道
#define STREAM_END_SIZE		0


enum {
	MODE420 = 0,
	MODE422 = 1,
	MODE224 = 2,
	MODE444 = 3,
	MODE400 = 4
};

struct frame_buf {
	unsigned long addrY;
	unsigned long addrCb;
	unsigned long addrCr;
	unsigned long strideY;
	unsigned long strideC;
	unsigned long mvColBuf;
	vpu_mem_desc desc;
};


void framebuf_init(void);
struct frame_buf *framebuf_alloc(int stdMode, int format, int strideY, int height, int mvCol);
void framebuf_free(struct frame_buf *fb,int iWidth,int iHeight);


#endif

