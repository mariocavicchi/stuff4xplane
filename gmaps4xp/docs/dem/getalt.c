 
// ftp://xftp.jrc.it/pub/srtmV4/tiff/srtm_39_04.zip

#include <stdio.h>
#include <gdal.h>

int main(int argc, char **argv){
	int i;
	GDALDatasetH    hSrcDS;
	GDALRasterBandH hBandSrc;
	int             pxSizeX         	= 0;
	int             pxSizeY         	= 0;
	double          padfGeoTransform[6]	= {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	double          revfGeoTransform[6]	= {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	double		lat			= 0.0;
	double		lon			= 0.0;
	double		x			= 0.0;
	double		y			= 0.0;
	short int	value			= 0;

	GDALAllRegister();
	CPLSetErrorHandler(NULL);


	if ( argc < 3 ){
		printf("Usage: getalt SRTM_GTIFF LON LAT\n");
		return 1;
	}


	if ( (hSrcDS = GDALOpen( argv[1], GA_ReadOnly )) == NULL ){
		printf("Unable open src file %s\n", argv[1]);
		return 2;
	}


        if ( GDALGetGeoTransform( hSrcDS, padfGeoTransform ) == CE_Failure ){
                printf("Image no transform can be fetched...\n");
	        return 3;
        }


	if ( GDALInvGeoTransform(padfGeoTransform,revfGeoTransform) == FALSE ){
	        printf("Problem with inversion of matrix\n");
	        return 3;
	}
	pxSizeX	 = GDALGetRasterXSize(hSrcDS);
	pxSizeY  = GDALGetRasterYSize(hSrcDS);
        hBandSrc = GDALGetRasterBand( hSrcDS, 1 );


	for ( i = 2; i < argc; i+=2){
		lon = atof(argv[i]);
		lat = atof(argv[i+1]);

		GDALApplyGeoTransform(revfGeoTransform, lon, lat, &x, &y);
		if ( x < 0 ) 		return 5;
		if ( y < 0 ) 		return 5;
		if ( x >= pxSizeX ) 	return 5;
		if ( y >= pxSizeX ) 	return 5;
		

		GDALRasterIO( hBandSrc, GF_Read, (int)x, (int)y, 1, 1, &value, 1, 1, GDT_Int16, 0, 0);
		if ( value == -32768 ) value = 0; // Water

		printf("%d ", value);
	}
	printf("\n");
	return 0;
}
