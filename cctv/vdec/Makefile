include ../../../../build.cfg


TAR_FILE=libvdec.a

RESUME_DIR_FILES += -path ./demo -prune -o

SRC_FILES = $(shell find ./  $(RESUME_DIR_FILES) -name "*.c" -print)
OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))


#SRC_FILES=$(shell find ./ -name "*.c" -print)
#OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))


INC_PATH=./include -I../include/

CFLAGS+=-I$(INC_PATH)

all:$(TAR_FILE)
$(TAR_FILE):$(OBJ_FILES)
	$(AR) -rc $(TAR_FILE) $(OBJ_FILES)
	cp $(TAR_FILE) ../CCTV/lib/


install:
	cp $(TAR_FILE) ../CCTV/lib/
clean:
	rm -rf $(OBJ_FILES)
	rm -rf $(TAR_FILE)
