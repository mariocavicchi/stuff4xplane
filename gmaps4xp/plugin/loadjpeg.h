#ifndef _LOADJPEG_H
#define _LOADJPEG_H


#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h> 
#include "jpeg_memory_src.h"
#define SCALE	1

int loadJpeg(unsigned char *ram, FILE *fd, int len,  unsigned char **image, int *width, int *height);

#endif
