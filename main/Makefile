include ../../../build.cfg

TAR_FILE=cctv.exe

ifeq ($(ARCH_NAME), arm)
SRC_FILES=main.cxx
OBJ_FILES=main.o
TAR_FILE=cctv.exe
else
SRC_FILES=main_pc.cxx
OBJ_FILES=main_pc.o
TAR_FILE=cctv_pc.exe
endif

ifeq ($(ARCH_NAME), arm)
LIB_PATH =../lib/ -L../CCTV/lib
LIB_PATH +=-L$(XORGPATH)/lib
else
LIB_PATH =../lib_pc/
LIB_PATH +=-L../HMI_DMI/lib_pc/
endif


ifeq ($(ARCH_NAME), arm)
LIB_FILES=-lcctv -lpmsg -lstate -lvpu_30101 -lXcursor -lXinerama -lfltk -lfltk_forms -lfltk_images -lfltk_jpeg -lfltk_png 
else
LIB_FILES=-lfltk -lfltk_forms -lfltk_images -ljpeg -lpng -lXcursor -lXinerama
endif

LIB_FILES+= -lXext -lXfixes\
-lxcb -lXau -lz -lm -lpthread -ldl -lXft -lX11 -lfontconfig -lXrender -lfreetype 

INC_PATH=../include/ -I../CCTV/include -I$(XORGPATH)/include 
CFLAGS+=-I$(INC_PATH) 
CXXFLAGS+=-I$(INC_PATH) -Wall -D__linux -rdynamic


all: $(RM) $(TAR_FILE)
$(RM):
	rm -rf $(TAR_FILE)
	
$(TAR_FILE):$(OBJ_FILES)
	$(CXX)  -o $(TAR_FILE) $(OBJ_FILES)  $(CXXFLAGS) -L$(LIB_PATH) $(LIB_FILES)	   
#$(TAR_FILE):$(SRC_FILES)
#	$(CXX) $(SRC_FILES) -o $(TAR_FILE)  $(CXXFLAGS) -L$(LIB_PATH) $(LIB_FILES)	   
#	cp $(TAR_FILE) ../bin

$(OBJ_FILES):$(SRC_FILES)
	$(CXX) -c $(SRC_FILES) $(CXXFLAGS) 

install:
	cp $(TAR_FILE) $(IMAGE_DIR)/userfs/bin
	cp -rf ../CCTV/res $(IMAGE_DIR)/userfs/bin/

clean:
	rm -rf $(OBJ_FILES)
	rm -rf $(TAR_FILE)
