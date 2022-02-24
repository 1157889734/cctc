#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "../include/types.h"
#include "../include/msg.h"
#include "../include/msgapp.h"
#include "event.h"

#define MAX_BUF_LEN 10240

static char g_acBuf[MAX_BUF_LEN];
static int g_iThreadExitFlag = 0;


int LMSG_SendMsgToDHMI(int iCmd, char *pcBuf, int iLen)
{
    T_MSG_APP_DATA tAppData;
    T_MSG tMsg;
    T_SOCK_DATA tSockData;
    T_SOCK_ADDR tSockAddr;
    struct sockaddr_in sa;
    int iRet = 0;

    tAppData.iCmd = iCmd;
    if (iLen > 0)
    {
        memcpy(tAppData.acData, pcBuf, iLen);
        tAppData.iLen = iLen;
    }
    else
    {
        tAppData.iLen = 0; 
    }

    tSockData.pData = (void *)&tAppData;
    tSockData.iLen = sizeof(int) + sizeof(int) + tAppData.iLen;

    EVT_GetSockAddr(SOCKET_TYPE_DHMI_CTRL, &sa);
   // printf("addr %x port %d\n", sa.sin_addr.s_addr, ntohs(sa.sin_port));
    tSockAddr.pAddr = &sa;
    tSockAddr.iAddrLen = sizeof(sa);
    tMsg.iMsgToken = MSG_TOKEN_CCTV_ASYNC_UDP;
    tMsg.ptSockData = &tSockData;
    tMsg.ptSockAddr = &tSockAddr;

    iRet = MSG_SendMessage(&tMsg);
    return iRet;
}


static void *RecvMsgThread(void *arg)
{    
    T_MSG tMsg;
    T_SOCK_DATA tSockData;
    T_SOCK_ADDR tSockAddr;
    struct sockaddr_in sa;
    int iRet = 0;
    
    pthread_detach(pthread_self()); 
    
    while(g_iThreadExitFlag)
    {
        memset(&tMsg, 0, sizeof(tMsg));
        tSockData.pData = g_acBuf;
        tSockData.iLen = sizeof(g_acBuf);
        tSockAddr.pAddr = &sa;
        tSockAddr.iAddrLen = sizeof(sa);
        tMsg.ptSockData = &tSockData;
        tMsg.ptSockAddr = &tSockAddr;
        
        iRet = MSG_ReadMessage (&tMsg);
        if (iRet == MSG_OPT_SUCCESS)
        {
            MSG_OnMessage (&tMsg);
        }
    }

}

int LMSG_Init(void)
{
    pthread_t tid;
    int iRet = 0;
    
    MSG_Init(MULTIPLE_THREAD);
    EVT_Init();
    g_iThreadExitFlag = 1;
    iRet = pthread_create(&tid, NULL, RecvMsgThread, NULL);
    
    return iRet;
}

int LMSG_Uninit(void)
{
    g_iThreadExitFlag = 0;
    sleep(1);
    MSG_UnInit();
    EVT_Uninit();
    
    return 0;
}

