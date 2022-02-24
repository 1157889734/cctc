#ifndef _CCTV_H_
#define _CCTV_H_

#ifdef __cplusplus
extern "C"{
#endif 

//1:只显示DMI 2:只显示HMI 3:CCTV
int  GetDevDispConfig();
int  main_cctv(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif 

	
#endif
