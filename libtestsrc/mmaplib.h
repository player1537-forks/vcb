#ifndef _MMAP_LIB_H
#define _MMAP_LIB_H


#ifdef __cplusplus
extern "C" {
#endif

char * mmap_infile(char * filename, int * binfile_len);
char * mmap_outfile(char * filename, int binfile_len);


#ifdef __cplusplus
}  /* extern C */
#endif

#endif
