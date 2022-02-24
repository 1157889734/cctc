#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "CCTV.h"
#include "state.h"


int main(int argc, char **argv)
{
	int iDisplayConfig = GetDevDispConfig();
	char cTestEnableFlg = 0;
	int iDispWindows = 1;

	GetTestEnableFlg(&cTestEnableFlg);
   
	if((1 == cTestEnableFlg) || (0 != STATE_FindUsbDev()))
    {
        iDispWindows = 0;
    }

	if(DISP_STATE_CCTV != iDisplayConfig)
	{
		iDispWindows = 0;
	}

	while(0 == iDispWindows)
	{
		sleep(10);
		if(DISP_STATE_CCTV == iDisplayConfig)
		{
			static char acBuf[128] = {0};
			memset(acBuf, 0, 128);
			ExecSysCmd("ps | grep -w PlatformForScreenTest.exe | grep -v grep", acBuf, 128);
			if(strlen(acBuf) > 0)
			{

			}
			else
			{
				system("/home/user/bin/PlatformForScreenTest.exe -qws");
			}
		}
	}
	
    main_cctv(argc,argv);

    while(1)
	{
		Fl::wait(1e20);
	}
	//Fl::run();
	return 0;
}

