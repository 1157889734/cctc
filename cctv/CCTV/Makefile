include ../../../../build.cfg

TAR_FILE=libcctv.a
TAR_LIB_FILE=libcctvwindos.a
HEAD_FILE=CCTV.h

OBJ_DIR          = obj

RESUME_DIR_FILES += -path ./demo -prune -o

SRC_FILES = $(shell find ./  $(RESUME_DIR_FILES) -name "*.cxx" -print)
OBJ_FILES=$(patsubst %.cxx,$(OBJ_DIR)/%.o,$(SRC_FILES))


LIB_CMP=./lib/libCMPlayer.a
LIB_RTSP=./lib/librtsp.a
LIB_VDEC=./lib/libvdec.a
LIB_FTP=./lib/libftp.a
LIB_DEBUGOUT=./lib/libdebugout.a
LIB_LOG=./lib/liblog.a
LIB_EVENT=./lib/libevent.a
LIB_MSG=./lib/libmsg.a

INC_PATH=../../include -I../include

CXXFLAGS+=-I$(INC_PATH)
CXXFLAGS+=-I$(XORGPATH)/include


#all: $(TAR_FILE)
all:$(TAR_LIB_FILE) $(TAR_FILE)


$(TAR_LIB_FILE):$(OBJ_FILES)
	$(AR) -rc $(TAR_LIB_FILE) $(OBJ_FILES)
	#cp $(TAR_FILE) ../lib

$(TAR_FILE):$(TAR_LIB_FILE)
	$(AR) -x $(LIB_CMP)
	$(AR) -x $(LIB_RTSP)
	$(AR) -x $(LIB_VDEC)
	$(AR) -x $(LIB_FTP)
	$(AR) -x $(LIB_DEBUGOUT)
	$(AR) -x $(LIB_LOG)
	$(AR) -x $(LIB_MSG)
	$(AR) -x $(LIB_EVENT)
	$(AR) -rcuv $(TAR_FILE) $(OBJ_FILES) *.o
	rm -f *.o
	cp $(TAR_FILE) ../../lib
	cp $(HEAD_FILE) ../../include

$(OBJ_DIR)/%.o : %.cxx
	@-mkdir -p $(OBJ_DIR)
	$(CXX) $(MK_FLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) -c  $< -o $@

install:
	cp $(TAR_FILE) ../lib/
clean:
	@echo $(OBJ_FILES)
	rm -rf $(OBJ_FILES)
	rm -rf $(TAR_FILE)
