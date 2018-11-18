SHELL = /bin/sh
                                                                                          
C++ = g++
CC  = gcc

ifeq ($(BUILD),debug)
CCFLAGS := -g
else
CCFLAGS := -O3
endif
INCLUDE = -I. -I../include -I/usr/include/GL
                                                                                          
LIB_PATH = -L../lib
                                                                                          
LIBS = -lm -lvcb
                                                                                          
EXECUTABLES = t_lic_main t_spcurve_main t_tensor_main t_artery_main \
	t_genmesh_main t_mcube_main t_rcast_main t_genimg_main \
	t_deri_avg_main t_slicer_main convertvol t_splat_main \
	t_strmln_main t_pca_main t_main_mqr t_cluster_main \
	t_amcube_main t_amcube_main_f t_btree_main t_cached_mcube_main \
	t_volfun_main t_voxel_line_main t_brain_line_main
                                                                                          
.SUFFIXES: .cc .c .cpp
                                                                                          
.cc.o:
	$(C++) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
.cpp.o:
	$(C++) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
.c.o:
	$(CC) $(CCFLAGS) $(INCLUDE) -c $<
                                                                                          
all: $(EXECUTABLES)
                                                                                          
t_amcube_main: t_amcube_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_amcube_main.o formain.o $(LIB_PATH) $(LIBS)

t_amcube_main_f: t_amcube_main_f.o formain.o
	$(CC) -o $@ $(CFLAGS) t_amcube_main_f.o formain.o $(LIB_PATH) $(LIBS)

t_artery_main: t_artery_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_artery_main.o formain.o $(LIB_PATH) $(LIBS)

t_brain_line_main: t_brain_line_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_brain_line_main.o formain.o $(LIB_PATH) $(LIBS)

t_btree_main: t_btree_main.o
	$(CC) -o $@ $(CFLAGS) t_btree_main.o $(LIB_PATH) $(LIBS)

t_cached_mcube_main: t_cached_mcube_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_cached_mcube_main.o formain.o $(LIB_PATH) $(LIBS)
	
t_cluster_main: t_cluster_main.o
	$(CC) -o $@ $(CFLAGS) t_cluster_main.o $(LIB_PATH) $(LIBS)

t_deri_avg_main: t_deri_avg_main.o 
	$(CC) -o $@ $(CFLAGS) t_deri_avg_main.o $(LIB_PATH) $(LIBS)

t_genmesh_main: t_genmesh_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_genmesh_main.o formain.o $(LIB_PATH) $(LIBS)

t_genimg_main: t_genimg_main.o
	$(CC) -o $@ $(CFLAGS) t_genimg_main.o $(LIB_PATH) $(LIBS)

bin2bmp: bin2bmp.o
	$(CC) -o $@ $(CFLAGS) bin2bmp.o $(LIB_PATH) $(LIBS)

vcbcomp: vcbcomp.o
	$(CC) -o $@ $(CFLAGS) vcbcomp.o $(LIB_PATH) $(LIBS)

t_lic_main: t_lic_main.o formain.o 
	$(CC) -o $@ $(CFLAGS) t_lic_main.o formain.o $(LIB_PATH) $(LIBS)

t_main_mqr: t_main_mqr.o
	$(CC) -o $@ $(CFLAGS) t_main_mqr.o $(LIB_PATH) $(LIBS)

t_mcube_main: t_mcube_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_mcube_main.o formain.o $(LIB_PATH) $(LIBS)

t_pca_main: t_pca_main.o
	$(CC) -o $@ $(CFLAGS) t_pca_main.o $(LIB_PATH) $(LIBS)

t_rcast_main: t_rcast_main.o
	$(CC) -o $@ $(CFLAGS) t_rcast_main.o $(LIB_PATH) $(LIBS)

t_rcastm_main: t_rcastm_main.o mmaplib.o
	$(CC) -o $@ $(CFLAGS) t_rcastm_main.o mmaplib.o $(LIB_PATH) $(LIBS)

t_rcastb_main: t_rcastb_main.o mmaplib.o
	$(CC) -o $@ $(CFLAGS) t_rcastb_main.o mmaplib.o $(LIB_PATH) $(LIBS)

t_mcubem_main: t_mcubem_main.o mmaplib.o
	$(CC) -o $@ $(CFLAGS) t_mcubem_main.o mmaplib.o $(LIB_PATH) $(LIBS)

vcbmcube: tvcbmcube.o
	$(CC) -o $@ $(CFLAGS) tvcbmcube.o $(LIB_PATH) $(LIBS)

vcbrcast: vcbrcast.o
	$(CC) -o $@ $(CFLAGS) vcbrcast.o $(LIB_PATH) $(LIBS)

t_slicer_main: t_slicer_main.o
	$(CC) -o $@ $(CFLAGS) t_slicer_main.o $(LIB_PATH) $(LIBS)

t_spcurve_main: t_spcurve_main.o formain.o 
	$(CC) -o $@ $(CFLAGS) t_spcurve_main.o formain.o $(LIB_PATH) $(LIBS)

t_splat_main: t_splat_main.o formain.o 
	$(CC) -o $@ $(CFLAGS) t_splat_main.o formain.o $(LIB_PATH) $(LIBS)

t_strmln_main: t_strmln_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_strmln_main.o formain.o $(LIB_PATH) $(LIBS)

t_tensor_main: t_tensor_main.o formain.o 
	$(CC) -o $@ $(CFLAGS) t_tensor_main.o formain.o $(LIB_PATH) $(LIBS)

t_volfun_main: t_volfun_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_volfun_main.o formain.o $(LIB_PATH) $(LIBS)

t_voxel_line_main: t_voxel_line_main.o formain.o
	$(CC) -o $@ $(CFLAGS) t_voxel_line_main.o formain.o $(LIB_PATH) $(LIBS)

convertvol: convertvol.o 
	$(CC) -o $@ $(CFLAGS) convertvol.o $(LIB_PATH) $(LIBS)

convertrmi: convertrmi.o
	$(CC) -o $@ $(CFLAGS) convertrmi.o $(LIB_PATH) $(LIBS)
                                                                                
convertvolm: convertvolm.o
	$(CC) -o $@ $(CFLAGS) convertvolm.o $(LIB_PATH) $(LIBS)

convertraw: convertraw.o
	$(CC) -o $@ $(CFLAGS) convertraw.o $(LIB_PATH) $(LIBS)

convertraw_ub: convertraw_ub.o
	$(CC) -o $@ $(CFLAGS) convertraw_ub.o $(LIB_PATH) $(LIBS)
	
vcbhead: vcbhead.o
	$(CC) -o $@ $(CFLAGS) vcbhead.o $(LIB_PATH) $(LIBS)

vcbrange: vcbrange.o
	$(CC) -o $@ $(CFLAGS) vcbrange.o $(LIB_PATH) $(LIBS)

addgrad: addgrad.o
	$(CC) -o $@ $(CFLAGS) addgrad.o $(LIB_PATH) $(LIBS)

vcbcutlarge: vcbcutlarge.o
	$(CC) -o $@ $(CFLAGS) vcbcutlarge.o $(LIB_PATH) $(LIBS)

clean:
	rm -f $(EXECUTABLES) *.o core *~

move:
	mv -f $(EXECUTABLES) ../libtest/
