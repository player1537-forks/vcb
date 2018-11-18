CPP = g++
CC = gcc

ifeq ($(BUILD),debug)
CFLAGS := -g
else
CFLAGS := -O3
endif

OBJS = MemRoot.o hashtable.o vcbcache.o \
	   jval.o jrb.o dllist.o

all: libvcbcache.a
dynamic: libvcbcache.so

libvcbcache.a : $(OBJS)
	$(RM) -f $@
	ar cq $@ $(OBJS) 
	ranlib $@

libvcbcache.so : $(OBJS)
	$(RM) -f $@
	ld -G $(OBJS) -o $@

.cpp.o: 
	$(CPP) $(CFLAGS) -c $<
	
.c.o: 
	$(CC) $(CFLAGS) -c $<

.PHONY: debug
debug: CFLAGS := $(CFLAGS) -DDEBUG
debug: clean all

libfdr.a: jval.c jval.h jrb.h jrb.c dllist.c dllist.h
	make -f makefile_fdr

clean:
	rm -f a.out *.o $(OBJS) libfdr.a libvcbcache.*
