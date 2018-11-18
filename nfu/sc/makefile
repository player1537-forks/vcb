SHELL = /bin/sh

C++ = g++
CC  = g++

CCFLAGS := -O3
DBGFLAGS := -g

INCLUDE = -I/System/Library/Frameworks/OpenGL.framework/Versions/A/Headers -I/sw/include/GL -I. -I../vcblib/include -I../vcblib/libtestsrc -I./GLUI/glui_v2_2

LIB_PATH =  -L/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries -L/sw/lib -L../vcblib/lib -L./GLUI/glui_v2_2

LIBS = -lm -lGLU -lGL -lglut -lvcbwithgl -lglui


EXECUTABLES = ibpmeshview ibprcastview

.SUFFIXES: .cc .c .cpp
                                                                                          
.cc.o:
	$(C++) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
.cpp.o:
	$(C++) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
.c.o:
	$(CC) $(CCFLAGS) $(INCLUDE) -c $<

all: $(EXECUTABLES)

ibprcastview: ibprcastview.o formainrcast.o
	$(CC) -o bin/$@ $(CFLAGS) ibprcastview.o formainrcast.o $(LIB_PATH) $(LIBS) 

ibprcastviewdbg: ibprcastview.o formainrcast.o
	$(CC) -o bin/$@ $(DBGFLAGS) ibprcastview.o formainrcast.o $(LIB_PATH) $(LIBS)

ibpmeshview: ibpmeshview.o formain.o
	$(CC) -o bin/$@ $(CFLAGS) ibpmeshview.o formain.o $(LIB_PATH) $(LIBS)

ibpmeshviewdbg: ibpmeshview.o formain.o
	$(CC) -o bin/$@ $(DBGFLAGS) ibpmeshview.o formain.o $(LIB_PATH) $(LIBS)

clean:
	/bin/rm *.o *~ *dbg ibpmeshview ibprcastview
