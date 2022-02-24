CXX = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-fsl-linux-gnueabi-g++
#include ../../build.cfg
CXX_pc=g++
TAR_FILE= pc_FltkTest.exe
OBJEC_dg=fltkTest.exe
CFLAGS = -Wall\
	-DG_DISABLE_DEPRECATED 	 	\
	-DGDK_DISABLE_DEPRECATED 	\
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED \
  	-Wno-deprecated 

XORGPATH=/home/niel/fltk/xorg-arm

libDirs_dg	= -L../../lib\
		-L$(XORGPATH)/lib
		#-L../mvb_buf\
		
incDirs_dg	=	-I../../include \
		-I$(XORGPATH)/include
		#-I../mvb_buf\
		 
#LIBS	=	-lX11 -lXext -lfltk -lfltk_images -lfltk_jpeg -lz -lm -lpthread -lPixyMvb -lrt -lsqlite3 #-ldata_buf_ppc\

libDirs_pc	=	
		
incDirs_pc	= -I./state
		#-I../mvb_buf\

LIBS_pc	=	-lfltk_images -lfltk -lpng -ljpeg -lz\
			-lXrender -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lpthread  -lX11 -lpthread -ldl -lm 
			
				
LIBS_dg	= -lfltk -lfltk_forms -lfltk_images -lfltk_jpeg -lfltk_png  -lX11 -lXext -lXfixes -lxcb\
			 -lXau -lz -lm -lpthread -ldl -lXft -lfontconfig -lXrender -lfreetype -lXcursor -lXinerama

SRCs	= main.cxx
##debug.cxx log.cxx fileConfig.cxx  state.cxx  MyPlayBoxWid.cxx CCTVWid.cxx
			
all: $(RM) $(TAR_FILE)        	
$(RM):
	rm  -f $(TAR_FILE)
$(TAR_FILE):$(SRCs)
	$(CXX_pc) $(SRCs) -o $(TAR_FILE) $(CFLAGS) $(libDirs_pc) $(incDirs_pc) $(LIBS_pc)	 

dg:
	$(CXX) $(SRCs) -o $(OBJEC_dg) $(CFLAGS)   $(libDirs_dg) $(incDirs_dg) $(LIBS_dg)		
       

clean: 
	rm -f *.o $(OBJECT)
