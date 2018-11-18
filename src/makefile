SHELL = /bin/sh
                                                                                          
LIBNAME = vcb
RM = /bin/rm
AR = ar cq
                                                                                          
TOP = ..
C++ = g++
CC  = gcc


ifeq ($(BUILD),debug)
CCFLAGS := -g
else
CCFLAGS := -O3
endif
                                                                                          
INCLUDE = -I. -I../include -I../Task -I/usr/include/GL
                                                                                          
LIBS = -lm

OBJS = ArcBall.o Lic.o dmem.o exhash.o fields.o glew.o hilbert.o intersect.o kernel.o nr.o nrcomplex.o nrutil.o raycasting.o slic.o slic_aux.o splatting.o vbutrcast.o vcbbtree.o vcbcluster.o vcbcolor.o vcbdebug.o vcbfitting.o vcbglutils.o vcbhist.o vcbimage.o vcblic.o vcblinalg.o vcbmath.o vcbmcube.o vcbmntree.o vcbops.o vcbpbuffer.o vcbrange.o vcbspcurve.o vcbtrimesh.o vcbutils.o vcbvolren.o zcurve.o

.SUFFIXES: .cc .c .cpp
                                                                                          
.cc.o:
	$(C++) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
.cpp.o:
	$(C++) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
.c.o:
	$(CC) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
all: lib$(LIBNAME).a
dynamic: lib$(LIBNAME).so
                                                                                          
lib$(LIBNAME).a : $(OBJS)
	$(RM) -f $@
	$(AR) $@ $(OBJS)
lib$(LIBNAME).so : $(OBJS)
	$(RM) -f $@
	ld -G $(OBJS) -o $@

clean:
	rm -f *.o *.a *.so
