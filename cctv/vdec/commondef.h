#ifndef  __COMMON__DEF_H__
#define  __COMMON__DEF_H__

#include <pthread.h>
#include "debug.h"

typedef struct _T_WINDOW_RECT
{
	int iLeft;
	int iTop;
	int iWidth;
	int iHeight;
}T_WINDOW_RECT;

#define err_msg(fmt, arg...)   { DebugPrint(DEBUG_ERROR_PRINT,"[ERR] %s:%d" fmt, __FUNCTION__, __LINE__,## arg);} 

#define info_msg(fmt, arg...)  { DebugPrint(DEBUG_ERROR_PRINT,"[INFO] %s:%d" fmt,__FUNCTION__, __LINE__, ## arg);} 

#define warn_msg(fmt, arg...)  { DebugPrint(DEBUG_ERROR_PRINT,"[WARN] %s:%d " fmt,  __FUNCTION__, __LINE__, ## arg);} 

typedef struct _T_DATA_PACKET
{
	char *pcData;
	int iLen;
    int iIFrameFlag;
}T_DATA_PACKET, *PT_DATA_PACKET;

typedef struct _T_DATA_PACKET_LIST
{
	T_DATA_PACKET tPkt;
	struct _T_DATA_PACKET_LIST *next;
}T_DATA_PACKET_LIST, *PT_DATA_PACKET_LIST;

typedef struct _T_PACKET_QUEUE {
	T_DATA_PACKET_LIST *first_pkt, *last_pkt;
	int nb_packets;
	pthread_mutex_t hMutex;

}T_PACKET_QUEUE;


void packet_queue_init(T_PACKET_QUEUE *q);
void packet_queue_flush(T_PACKET_QUEUE *q);
void packet_queue_uninit(T_PACKET_QUEUE *q);
int packet_queue_put(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt);
int packet_queue_get(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt, int block);
int packet_queue_get_packet_num(T_PACKET_QUEUE *q);
int packet_queue_get_first_IFrame(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt);

#endif