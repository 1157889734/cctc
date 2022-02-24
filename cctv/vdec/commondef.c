#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "commondef.h"

void packet_queue_init(T_PACKET_QUEUE *q)
{
	if(NULL==q)
	{
		return;
	}
	memset(q, 0, sizeof(T_PACKET_QUEUE));
#ifdef _WINDOWS_
	q->hMutex = CreateMutex( NULL, false, NULL );
#else
	pthread_mutex_init(&q->hMutex,NULL);  
#endif
}

void packet_queue_flush(T_PACKET_QUEUE *q)
{
	PT_DATA_PACKET_LIST pkt, pkt1;
	if(NULL==q)
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
		if((NULL!=pkt) && (NULL!=pkt->tPkt.pcData))
		{
			free(pkt->tPkt.pcData);
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

void packet_queue_uninit(T_PACKET_QUEUE *q)
{
	if(q)
	{
		packet_queue_flush(q);
#ifdef _WINDOWS_
		CloseHandle(q->hMutex);
#else
		pthread_mutex_destroy(&q->hMutex );   
#endif
	}
}

int packet_queue_put(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt)
{
	PT_DATA_PACKET_LIST pktl = NULL;

	if (NULL == pkt)
		return -1;

	pktl = (PT_DATA_PACKET_LIST)malloc(sizeof(T_DATA_PACKET_LIST));
	if (!pktl)
		return -1;
	pktl->tPkt = *pkt;
	pktl->next = NULL;

	while(1)
	{
#ifdef _WINDOWS_
		WaitForSingleObject( q->hMutex, INFINITE );
#else
		pthread_mutex_lock( &q->hMutex );
#endif
		if (q->nb_packets > 100)
		{
#ifdef _WINDOWS_
			ReleaseMutex( q->hMutex);
#else
			pthread_mutex_unlock( &q->hMutex );
#endif
			usleep(5000);
		}
		else
		{
#ifdef _WINDOWS_
			ReleaseMutex( q->hMutex);
#else
			pthread_mutex_unlock( &q->hMutex );
#endif
			break;
		}
	}

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

int packet_queue_put_front(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt)
{
	PT_DATA_PACKET_LIST pktl = NULL;

	if (NULL == pkt)
		return -1;

	pktl = (PT_DATA_PACKET_LIST)malloc(sizeof(T_DATA_PACKET_LIST));
	if (!pktl)
		return -1;
	pktl->tPkt = *pkt;
	pktl->next = NULL;

#ifdef _WINDOWS_
	WaitForSingleObject( q->hMutex, INFINITE );
#else
	pthread_mutex_lock( &q->hMutex );
#endif

	pktl->next = q->first_pkt;
	q->first_pkt = pktl->next;
	q->nb_packets++;

#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif
	return 0;
}

int packet_queue_get(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt, int block)
{
	PT_DATA_PACKET_LIST pktl;
	int ret =0;
	if(NULL==q)
	{
		return ret;
	}
#ifdef _WINDOWS_
	WaitForSingleObject( q->hMutex, INFINITE );
#else
	pthread_mutex_lock( &q->hMutex );
#endif
	for(;;) 
	{
		pktl = q->first_pkt;
		if (pktl) {
			q->first_pkt = pktl->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			*pkt = pktl->tPkt;
			free(pktl);
			ret = 1;
			// DebugPrint(3,"[%s %d] pktl!=NULL",__FUNCTION__, __LINE__);
			break;
		} else if (!block) {
			// DebugPrint(3,"[%s %d] block == NULL",__FUNCTION__, __LINE__);
			ret = 0;
			break;
		} else {
#ifdef _WINDOWS_
			ReleaseMutex( q->hMutex);
#else
			pthread_mutex_unlock( &q->hMutex );
#endif
			usleep(2000);
#ifdef _WINDOWS_
			WaitForSingleObject( q->hMutex, INFINITE );
#else
			pthread_mutex_lock( &q->hMutex );
#endif
		}
	}
#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif
	return ret;
}

int packet_queue_get_packet_num(T_PACKET_QUEUE *q)
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


PT_DATA_PACKET_LIST FindIFrameFromeQueue(T_PACKET_QUEUE *q)
{
    PT_DATA_PACKET_LIST ptList = NULL;
    PT_DATA_PACKET_LIST ptIFrameList = NULL;

#ifdef _WINDOWS_
	WaitForSingleObject( q->hMutex, INFINITE );
#else
	pthread_mutex_lock( &q->hMutex );
#endif

    ptList = q->first_pkt;
    while (ptList)
    {
         if (ptList->tPkt.iIFrameFlag)
         {
             ptIFrameList = ptList;
         }

         ptList = ptList->next;
    }
#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif

    return ptIFrameList;
}

int packet_queue_get_first_IFrame(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt)
{
    PT_DATA_PACKET_LIST ptList = NULL, ptListTmp;
    PT_DATA_PACKET_LIST ptIFrameList = NULL;
    int iRet = 0;

    ptIFrameList = FindIFrameFromeQueue(q);
    if (ptIFrameList)
    {
#ifdef _WINDOWS_
    WaitForSingleObject( q->hMutex, INFINITE );
#else
    pthread_mutex_lock( &q->hMutex );
#endif

        ptList = q->first_pkt;
        while (ptList)
        {
            ptListTmp = ptList->next;
            if (ptList != ptIFrameList)
            {
                free(ptList->tPkt.pcData);
                free(ptList);

                q->first_pkt = ptListTmp;
                if (!q->first_pkt)
                {
                    q->last_pkt = NULL;
                }
                q->nb_packets--;
            }
            else
            {
                q->first_pkt = ptListTmp;
                if (!q->first_pkt)
				q->last_pkt = NULL;
                q->nb_packets--;
                *pkt = ptList->tPkt;
				
                free(ptList);
                iRet = 1;
                break;
            }

            ptList = ptListTmp;
        }
		
#ifdef _WINDOWS_
	ReleaseMutex( q->hMutex);
#else
	pthread_mutex_unlock( &q->hMutex );
#endif
	}
	else
	{
	    packet_queue_flush(q);
        return 0;
	}

	return iRet;
}

/*
int packet_queue_get_firstIFrame(T_PACKET_QUEUE *q, PT_DATA_PACKET pkt)
{
    PT_DATA_PACKET_LIST pktTmp = NULL;
    int iRet = 0;
    if(NULL == q)
    {
         return 0;
    }
#ifdef _WINDOWS_
        WaitForSingleObject( q->hMutex, INFINITE );
#else
        pthread_mutex_lock( &q->hMutex );
#endif

     pktTmp = q->first_pkt;
        
     while(pktTmp) 
	 {
	     PT_DATA_PACKET_LIST pktTmp2 = pktTmp->next;
         
    	 if(pktTmp->tPkt.iIFrameFlag)
         {
               PT_DATA_PACKET_LIST pktTmp3 = q->first_pkt;
  
                while(pktTmp != pktTmp3 && pktTmp3)
               {
                    q->first_pkt = pktTmp3->next;
                    free(pktTmp3->tPkt.pcData);
                    free(pktTmp3);
                    pktTmp3 = q->first_pkt;
                    q->nb_packets--;
               }
               iRet = 1;
         }  
         pktTmp = pktTmp2;
    }

    if(1 == iRet)
    {
        pktTmp = q->first_pkt;
        q->first_pkt = pktTmp->next;
		if (!q->first_pkt)
			q->last_pkt = NULL;
		q->nb_packets--;
		*pkt = pktTmp->tPkt;
		free(pktTmp);
    }
     
#ifdef _WINDOWS_
        ReleaseMutex( q->hMutex);
#else
        pthread_mutex_unlock( &q->hMutex );
#endif

return iRet;
}
*/

