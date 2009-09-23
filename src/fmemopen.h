#ifndef __HERMES2D_FMEMOPEN_H
#define __HERMES2D_FMEMOPEN_H

#include "common.h"

/* The GNU libc fmemopen
   This function is available by default in GNU libc and on other platforms
   (like OSX or Cygwin) that don't provide this function, we implement our own version
   (e.g. mac/fmemopen.cpp).
 */
FILE *fmemopen (void *buf, size_t size, const char *opentype);

#endif
