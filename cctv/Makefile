
#include build.cfg


SUBDIRS= debugout log state vdec rtsp CMPlayer  ftp pmsg event CCTV 

all:
	@for dir in $(SUBDIRS); do \
	echo "making all in $$dir";\
	(cd $$dir;make all)||exit 1;\
	done

install:
	@for dir in $(SUBDIRS); do \
	echo "making install in $$dir";\
	(cd $$dir;make install)||exit 1;\
	done
	

clean:
	@for dir in $(SUBDIRS); do \
        echo "making clean in $$dir";\
        (cd $$dir;make clean)||exit 1;\
        done
