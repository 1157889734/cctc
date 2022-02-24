#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "types.h"
#include "event.h"
#include "state.h"
#include "msgapp.h"
#include "msg.h"


int OnMsgDhmiRequestState(int iCmd, char *pcData, long lParam);
int OnMsgDhmiSwitchCCTV(int iCmd, char *pcData, long lParam);
int OnMsgDhmiSwitchDMI(int iCmd, char *pcData, long lParam);
int OnMsgDhmiSwitchHMI(int iCmd, char *pcData, long lParam);
int OnMsgDhmiRespCctvRequestState(int iCmd, char *pcData, long lParam);


static T_EVENT_FUNC g_atEvtMap[] = {

// dhmi -> cctv
{MSG_DHMI2CCTV_ASYNC_REQUEST_STATE, OnMsgDhmiRequestState},
{MSG_DHMI2CCTV_ASYNC_SWITCH_CCTV, OnMsgDhmiSwitchCCTV},
{MSG_DHMI2CCTV_ASYNC_SWITCH_DMI, OnMsgDhmiSwitchDMI},
{MSG_DHMI2CCTV_ASYNC_SWITCH_HMI, OnMsgDhmiSwitchHMI},
{MSG_DHMI2CCTV_ASYNC_RESP_CCTV_REQUEST_STATE, OnMsgDhmiRespCctvRequestState},
};

int ProcessEvtMsg(int iCmd, char *pcData, long lParam)
{
    int i = 0;
    int iRet = 0;
    
    for (i = 0; i < sizeof(g_atEvtMap) / sizeof(T_EVENT_FUNC); i++)
    {
        if (g_atEvtMap[i].iCmd == iCmd)
        {
            if (g_atEvtMap[i].pfEvtFunc)
            {
                iRet = g_atEvtMap[i].pfEvtFunc(iCmd, pcData, lParam);
            }	
        }	
    }
    
    return iRet;
}

int OnMsgDhmiRequestState(int iCmd, char *pcData, long lParam)
{
    char cState = 0;

    cState = GetDisplayState();
    LMSG_SendMsgToDHMI(MSG_CCTV2DHMI_ASYNC_RESP_DHMI_REQUEST_STATE, &cState, 1);

    return EVT_OPT_SUCCESS;
}

int OnMsgDhmiSwitchCCTV(int iCmd, char *pcData, long lParam)
{
	SetDisplayState(DISP_STATE_CCTV);
    return EVT_OPT_SUCCESS;
}

int OnMsgDhmiSwitchDMI(int iCmd, char *pcData, long lParam)
{
	SetDisplayState(DISP_STATE_DMI);
    return EVT_OPT_SUCCESS;
}

int OnMsgDhmiSwitchHMI(int iCmd, char *pcData, long lParam)
{
	SetDisplayState(DISP_STATE_HMI);
    return EVT_OPT_SUCCESS;
}

int OnMsgDhmiRespCctvRequestState(int iCmd, char *pcData, long lParam)
{
    char cState = pcData[0];
      
    if (DISP_STATE_DMI == cState)
    {
        SetDisplayState(DISP_STATE_DMI);
    }
    else if (DISP_STATE_HMI == cState)
    {
        SetDisplayState(DISP_STATE_HMI);
    }
    else
    {
        SetDisplayState(DISP_STATE_CCTV);
    }
        
    return EVT_OPT_SUCCESS;
}
