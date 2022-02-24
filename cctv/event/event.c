#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "msg.h"
#include "msgapp.h"
#include "event.h"

typedef struct _T_EVT_INIT_INFO
{
    struct sockaddr_in tDhmiSockAddr;

}__attribute__((packed))T_EVT_INIT_INFO;

static T_EVT_INIT_INFO g_tEvtInitInfo;


extern int ProcessEvtMsg(int iCmd, char *pcData, long param);

int EVT_Process(PT_MSG pMsg)
{
    int iRet = 0;
    PT_MSG_APP_DATA pAppData = NULL;
		
    pAppData = (PT_MSG_APP_DATA)(pMsg->ptSockData->pData);
    iRet = ProcessEvtMsg(pAppData->iCmd, pAppData->acData, (long)(pMsg->ptSockAddr));
    
    return iRet;
}


int CreateCctvAsyncSocket(void)
{
    int flag = 1;
    int err;
    int iSockFd = 0;
    struct sockaddr_in tSockAddr;
	
    iSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSockFd <= 0)
    {
        printf("[%s]Create local socket Error.\n", __FUNCTION__);
        return EVT_FAIL_CREATE_SOCKET;
    }
	
    tSockAddr.sin_family      = AF_INET;
    tSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("127.0.0.1");
    tSockAddr.sin_port        = htons(CCTV_CTRL_ASYNC_PORT);
	
    err = setsockopt(iSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));
    if(err != 0)
    {
        printf("ERR: setsockopt vod socket error. err = %d,errno = %d[%s]\n",err, errno, strerror(errno));
        close(iSockFd);
        return EVENT_FAIL_SOCKET_BIND;
    }
    if (bind(iSockFd, (struct sockaddr*)&tSockAddr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("ERR:  bind error.\n");
        close(iSockFd);
        return EVENT_FAIL_SOCKET_BIND;
    }
    
    return iSockFd;
}

int RegCctvAsyncSocket(int iSockFd)
{
    T_MSG_REG_ITEM tItem = {0};
    int iRet = 0;

    tItem.iHandleType = MSG_HANDLE_TYPE_SOCKET_DATAGRAM;
    tItem.iMsgToken   = MSG_TOKEN_CCTV_ASYNC_UDP;
    tItem.iHandle     = iSockFd;
    tItem.pfMsgProc   = EVT_Process;
    
    iRet = MSG_RegHandle(&tItem);
    
    return iRet;
}


int EVT_Init(void)
{
    int iSockFd = 0;
    int iRet    = 0;

    iSockFd = CreateCctvAsyncSocket();
    if (iSockFd > 0)
    {
        iRet = RegCctvAsyncSocket(iSockFd);
    }

    g_tEvtInitInfo.tDhmiSockAddr.sin_port = htons(DHMI_CTRL_ASYNC_PORT);
    g_tEvtInitInfo.tDhmiSockAddr.sin_family = AF_INET;
    g_tEvtInitInfo.tDhmiSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    return iRet;
}

int EVT_Uninit(void)
{
    
    return EVT_OPT_SUCCESS;
}

int EVT_GetSockAddr(int iSocketType, void* pSocketAddr)
{
    int iRet = 0;

    if (pSocketAddr == NULL)
    {
        return -1;
    }
    switch (iSocketType)
    {
        case SOCKET_TYPE_DHMI_CTRL:
            memcpy((char *)pSocketAddr, (char *)(&g_tEvtInitInfo.tDhmiSockAddr),
                   sizeof(struct sockaddr_in));
            break;
        default:
            printf("Can not find the socket type\n");
            break;
    }
    return iRet;
}

