include ../../../../build.cfg

TAR_FILE=libevent.a
TAR_HEAD=lmsgProc.h

SRC_FILES=$(shell find ./ -name "*.c" -print)
OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))


INC_PATH=../../include/
INC_PATH+=-I../include/
INC_PATH+=-I../../../P1/include/
CFLAGS+=-I$(INC_PATH)

all:$(TAR_FILE)
$(TAR_FILE):$(OBJ_FILES)
	$(AR) -rc $(TAR_FILE) $(OBJ_FILES)
	cp $(TAR_FILE) ../CCTV/lib
	cp $(TAR_HEAD) ../include

install:
	cp $(TAR_FILE) ../lib/
clean:
	rm -rf $(OBJ_FILES)
	rm -rf $(TAR_FILE)
