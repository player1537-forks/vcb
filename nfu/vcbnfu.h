#ifndef _VCB_NFU_H
#define _VCB_NFU_H

#include "ibp.h"


/* NFU para structure */
typedef struct {
  void *data;
  int len;
}NFU_PARA;

#ifdef __cplusplus
extern "C" {
#endif


int NFUvcbMcube(int npara, NFU_PARA *paras);

int NFUvcbRcast(int npara, NFU_PARA * paras);


#ifdef __cplusplus
}  /* extern C */
#endif

#endif
