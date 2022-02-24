#ifndef _T_EVENT_H_
#define _T_EVENT_H_

#define EVENT_DEBUG_LEVEL  DEBUG_LEVEL_9

#define EVT_OPT_SUCCESS 0
#define EVT_FAIL_CREATE_SOCKET 0xFE000001
#define EVENT_FAIL_SOCKET_BIND 0xFE000002 


#define SOCKET_TYPE_DHMI_CTRL       0x1



typedef int  (*PF_EVENT_CALLBACK)(int cmd, char *pcData, long lParam);

typedef struct _T_EVENT_FUNC
{
    int iCmd;
    PF_EVENT_CALLBACK pfEvtFunc;	
}__attribute__((packed))T_EVENT_FUNC, *PT_EVENT_FUNC;

int EVT_Init(void);
int EVT_Uninit(void);


int EVT_GetSockAddr(int iSocketType, void* pSocketAddr);


#endif
