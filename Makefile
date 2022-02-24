
include ../../build.cfg

SUBDIRS=CCTV main
INSTALLDIR = main




all:
	@for dir in $(SUBDIRS); do \
	echo "making all in $$dir";\
	(cd $$dir;make all)||exit 1;\
	done
	
pc:	
	@for dir in $(SUBDIRS); do \
	echo "making all in $$dir";\
	(cd $$dir;make pc)||exit 1;\
	done

install:
	@for dir in $(INSTALLDIR); do \
	echo "making install in $$dir";\
	(cd $$dir;make install)||exit 1;\
	done
	

clean:
	@for dir in $(SUBDIRS); do \
        echo "making clean in $$dir";\
        (cd $$dir;make clean)||exit 1;\
        done
