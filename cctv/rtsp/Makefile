include ../../../../build.cfg

TAR_FILE=librtsp.a

SRC_FILES=$(shell find ./ -name "*.c" -print)
OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))

all:$(TAR_FILE)
$(TAR_FILE):$(OBJ_FILES)
	$(AR) -rc $(TAR_FILE) $(OBJ_FILES)
	cp $(TAR_FILE) ../CCTV/lib/

install:
	cp $(TAR_FILE) ../lib/
clean:
	rm -rf $(OBJ_FILES)
	rm -rf $(TAR_FILE)
