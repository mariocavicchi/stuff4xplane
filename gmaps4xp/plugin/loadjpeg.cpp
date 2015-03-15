#include "loadjpeg.h"

int loadJpeg(unsigned char *ram, FILE *fd, int len,  unsigned char **image, int *width, int *height){
	int i;
	int		depth		= 0;
	int		dimention	= 0;
	unsigned long	location	= 0;


	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr		jerr;
	JSAMPROW			row_pointer[1];


	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	if ( ram != NULL )	jpeg_memory_src(&cinfo, ram, len);
	else			jpeg_stdio_src(&cinfo, fd);

	jpeg_read_header(&cinfo, 0);

	cinfo.scale_num		= 1;
	cinfo.scale_denom	= SCALE;
	jpeg_start_decompress(&cinfo);

	*width		= cinfo.output_width;
	*height		= cinfo.output_height;
	depth		= cinfo.num_components; //should always be 3
	dimention	= (*width) * depth;


	(*image)	= (unsigned char *) malloc( (*height) * dimention );
	if ( (*image) == NULL ) return 1;

	row_pointer[0] = (unsigned char *) malloc( dimention );

	/* read one scan line at a time */
	while( cinfo.output_scanline < cinfo.output_height ){
		jpeg_read_scanlines( &cinfo, row_pointer, 1 );
		for( i = 0; i < dimention; i++, location++) (*image)[location] = row_pointer[0][i];
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return 0;
}

