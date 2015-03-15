#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
/*
sim/weather/cloud_type[0]		int	y	Cloud enumeration	The type of clouds for this cloud layer.Currently there are only 3 cloud layers, 0, 1 or 2. 
										Cloud types:<p>Clear = 0, High Cirrus = 1, Scattered = 2, Broken = 3, Overcast = 4, Stratus = 5 (740 and newer)
sim/weather/cloud_type[1]		int	y	Cloud enumeration	The type of clouds for this cloud layer.Currently there are only 3 cloud layers, 0, 1 or 2.
										Cloud types:<p>Clear = 0, High Cirrus = 1, Scattered = 2, Broken = 3, Overcast = 4, Stratus = 5 (740 and newer)
sim/weather/cloud_type[2]		int	y	Cloud enumeration	The type of clouds for this cloud layer.Currently there are only 3 cloud layers, 0, 1 or 2. 
										Cloud types:<p>Clear = 0, High Cirrus = 1, Scattered = 2, Broken = 3, Overcast = 4, Stratus = 5 (740 and newer)
sim/weather/cloud_coverage[0]		float	y	Coverage 0..6		todo
sim/weather/cloud_coverage[1]		float	y	Coverage 0..6		todo
sim/weather/cloud_coverage[2]		float	y	Coverage 0..6		todo
sim/weather/cloud_base_msl_m[0]		float	y	meters MSL >= 0		The base altitude for this cloud layer.
sim/weather/cloud_base_msl_m[1]		float	y	meters MSL >= 0		The base altitude for this cloud layer.
sim/weather/cloud_base_msl_m[2]		float	y	meters MSL >= 0		The base altitude for this cloud layer.
sim/weather/cloud_tops_msl_m[0]		float	y	meters >= 0		The tops for this cloud layer.
sim/weather/cloud_tops_msl_m[1]		float	y	meters >= 0		The tops for this cloud layer.
sim/weather/cloud_tops_msl_m[2]		float	y	meters >= 0		The tops for this cloud layer.
sim/weather/visibility_reported_m	float	y	meters >= 0		The reported visibility (e.g. what the METAR/weather window says).
sim/weather/rain_percent		float	y	[0.0 - 1.0]		The percentage of rain falling.
sim/weather/thunderstorm_percent	float	y	[0.0 - 1.0]		The percentage of thunderstorms present.
sim/weather/wind_turbulence_percent	float	y	[0.0 - 1.0]		The precentage of wind turbulence present.
sim/weather/barometer_sealevel_inhg	float	y	29.92 +- ....		The barometric pressure at sea level.
sim/weather/use_real_weather_bool	int	y	0,1			Whether a real weather file is in use.
sim/weather/wind_altitude_msl_m[0]	float	y	meters >= 0		The center altitude of this layer of wind in MSL meters.
sim/weather/wind_altitude_msl_m[1]	float	y	meters >= 0		The center altitude of this layer of wind in MSL meters.
sim/weather/wind_altitude_msl_m[2]	float	y	meters >= 0		The center altitude of this layer of wind in MSL meters.
sim/weather/wind_direction_degt[0]	float	y	[0 - 360)		The direction the wind is blowing from in degrees from true 0rth c lockwise.
sim/weather/wind_direction_degt[1]	float	y	[0 - 360)		The direction the wind is blowing from in degrees from true 0rth c lockwise.
sim/weather/wind_direction_degt[2]	float	y	[0 - 360)		The direction the wind is blowing from in degrees from true 0rth c lockwise.
sim/weather/wind_speed_kt[0]		float	y	kts >= 0		The wind speed in k0ts.
sim/weather/wind_speed_kt[1]		float	y	kts >= 0		The wind speed in k0ts.
sim/weather/wind_speed_kt[2]		float	y	kts >= 0		The wind speed in k0ts.
sim/weather/shear_direction_degt[0]	float	y	[0 - 360)		The direction for a wind shear, per above.
sim/weather/shear_direction_degt[1]	float	y	[0 - 360)		The direction for a wind shear, per above.
sim/weather/shear_direction_degt[2]	float	y	[0 - 360)		The direction for a wind shear, per above.
sim/weather/shear_speed_kt[0]		float	y	kts >= 0		The gain from the shear in k0ts.
sim/weather/shear_speed_kt[1]		float	y	kts >= 0		The gain from the shear in k0ts.
sim/weather/shear_speed_kt[2]		float	y	kts >= 0		The gain from the shear in k0ts.
sim/weather/turbulence[0]		float	y	[0 - 10]		A turbulence factor, 0-10, the unit is just a scale.
sim/weather/turbulence[1]		float	y	[0 - 10]		A turbulence factor, 0-10, the unit is just a scale.
sim/weather/turbulence[2]		float	y	[0 - 10]		A turbulence factor, 0-10, the unit is just a scale.
sim/weather/wave_amplitude		float	y	meters			Amplitude of waves in the water (height of waves)
sim/weather/wave_length			float	y	meters			Length of a single wave in the water
sim/weather/wave_speed			float	y	meters/second		Speed of water waves
sim/weather/wave_dir			int	y	degrees			Direction of waves.
sim/weather/temperature_sealevel_c	float	y	degrees C		The temperature at sea level.
sim/weather/dewpoi_sealevel_c		float	y	degrees C		The dew point at sea level.
sim/weather/thermal_percent		float	y	[0.0 - 1.0]		The percentage of thermal occurence in the area.
sim/weather/thermal_rate_ms		float	y	m/s >= 0		The climb rate for thermals.
sim/weather/thermal_altitude_msl_m	float	y	m MSL >= 0		The top of thermals in meters MSL.
sim/weather/runway_friction		float	y	??	T		he friction constant for runways (how wet they are).
*/

#define MAX_DATAREF 		200
#define FALSE			1
#define TRUE			0
#define HOST_WRF		"localhost:1234"
#define WRF_RESOLUTION		0.10

#define FIRST_START		0
#define READY_TO_DOWNLOAD	1
#define DOWNLOADING		2
#define READY_TO_READ		3
#define READY_TO_APPLY		4
#define WAITING_FOR_NEWS	5


XPLMDataRef use_real_weather_bool 	= NULL;
XPLMDataRef cloud_type[3]		= { NULL, NULL, NULL };
XPLMDataRef cloud_coverage[3]		= { NULL, NULL, NULL };
XPLMDataRef cloud_base_msl_m[3]		= { NULL, NULL, NULL };
XPLMDataRef cloud_tops_msl_m[3]		= { NULL, NULL, NULL };
XPLMDataRef visibility_reported_m 	= NULL;
XPLMDataRef rain_percent 		= NULL;
XPLMDataRef thunderstorm_percent	= NULL;	
XPLMDataRef wind_turbulence_percent	= NULL;
XPLMDataRef barometer_sealevel_inhg 	= NULL;
XPLMDataRef wind_altitude_msl_m[3]	= { NULL, NULL, NULL };
XPLMDataRef wind_direction_degt[3]	= { NULL, NULL, NULL };
XPLMDataRef wind_speed_kt[3]		= { NULL, NULL, NULL };
XPLMDataRef shear_direction_degt[3]	= { NULL, NULL, NULL };
XPLMDataRef shear_speed_kt[3]		= { NULL, NULL, NULL };
XPLMDataRef turbulence[3]		= { NULL, NULL, NULL };
XPLMDataRef wave_amplitude		= NULL;	
XPLMDataRef wave_length			= NULL;
XPLMDataRef wave_speed			= NULL;
XPLMDataRef wave_dir			= NULL;
XPLMDataRef temperature_sealevel_c	= NULL;
XPLMDataRef dewpoi_sealevel_c		= NULL;
XPLMDataRef thermal_percent		= NULL;
XPLMDataRef thermal_rate_ms		= NULL;
XPLMDataRef thermal_altitude_msl_m	= NULL;
XPLMDataRef runway_friction		= NULL;
XPLMDataRef latitude			= NULL;
XPLMDataRef longitude			= NULL;
XPLMDataRef elevation			= NULL;


struct datarefBox{
	int 	idx;
	float 	value;
	char 	*name;
} datarefBox;
struct  datarefBox *datarefs = NULL;

float 	latOld 		= -9999; 
float 	lonOld 		= -9999;
float 	eleOld 		= -9999;
int	eleWRF 		= 0;
int	downloading 	= FALSE;
float 	vis		= 0;
int 	STATUS		= FIRST_START;
pid_t 	pid;
float	*highLevels	= NULL;

int readWeatherData();



int printWeatherParams(){
	printf("---------------------------------------------------\n");
	printf("sim/weather/use_real_weather_bool	%d\n",	XPLMGetDatai( use_real_weather_bool	));
	printf("sim/weather/cloud_type[0] 		%d\n", 	XPLMGetDatai( cloud_type[0] 		));
	printf("sim/weather/cloud_type[1] 		%d\n", 	XPLMGetDatai( cloud_type[1]		));
	printf("sim/weather/cloud_type[2] 		%d\n", 	XPLMGetDatai( cloud_type[2]		));
	printf("sim/weather/cloud_coverage[0] 		%f\n", 	XPLMGetDataf( cloud_coverage[0]		));
	printf("sim/weather/cloud_coverage[1] 		%f\n", 	XPLMGetDataf( cloud_coverage[1]		));
	printf("sim/weather/cloud_coverage[2] 		%f\n", 	XPLMGetDataf( cloud_coverage[2]		));
	printf("sim/weather/cloud_base_msl_m[0] 	%f\n", 	XPLMGetDataf( cloud_base_msl_m[0]	));
	printf("sim/weather/cloud_base_msl_m[1] 	%f\n", 	XPLMGetDataf( cloud_base_msl_m[1]	));
	printf("sim/weather/cloud_base_msl_m[2] 	%f\n", 	XPLMGetDataf( cloud_base_msl_m[2]	));
	printf("sim/weather/cloud_tops_msl_m[0] 	%f\n", 	XPLMGetDataf( cloud_tops_msl_m[0]	));
	printf("sim/weather/cloud_tops_msl_m[1] 	%f\n", 	XPLMGetDataf( cloud_tops_msl_m[1]	));
	printf("sim/weather/cloud_tops_msl_m[2] 	%f\n", 	XPLMGetDataf( cloud_tops_msl_m[2]	));
	printf("sim/weather/visibility_reported_m 	%f\n", 	XPLMGetDataf( visibility_reported_m	));
	printf("sim/weather/rain_percent 		%f\n", 	XPLMGetDataf( rain_percent		));
	printf("sim/weather/thunderstorm_percent  	%f\n",  XPLMGetDataf( thunderstorm_percent	));		
	printf("sim/weather/wind_turbulence_percent  	%f\n",  XPLMGetDataf( wind_turbulence_percent	)); 	
	printf("sim/weather/barometer_sealevel_inhg  	%f\n",  XPLMGetDataf( barometer_sealevel_inhg	));	
	printf("sim/weather/wind_altitude_msl_m[0]  	%f\n",  XPLMGetDataf( wind_altitude_msl_m[0]	));	
	printf("sim/weather/wind_altitude_msl_m[1]  	%f\n",  XPLMGetDataf( wind_altitude_msl_m[1]	));	
	printf("sim/weather/wind_altitude_msl_m[2]  	%f\n",  XPLMGetDataf( wind_altitude_msl_m[2]	));	
	printf("sim/weather/wind_direction_degt[0]  	%f\n",  XPLMGetDataf( wind_direction_degt[0]	));	
	printf("sim/weather/wind_direction_degt[1]  	%f\n",  XPLMGetDataf( wind_direction_degt[1]	));	
	printf("sim/weather/wind_direction_degt[2]  	%f\n",  XPLMGetDataf( wind_direction_degt[2]	));	
	printf("sim/weather/wind_speed_kt[0]  		%f\n",  XPLMGetDataf( wind_speed_kt[0]		));		
	printf("sim/weather/wind_speed_kt[1]  		%f\n",  XPLMGetDataf( wind_speed_kt[1]		));		
	printf("sim/weather/wind_speed_kt[2]  		%f\n",  XPLMGetDataf( wind_speed_kt[2]		));		
	printf("sim/weather/shear_direction_degt[0]  	%f\n",  XPLMGetDataf( shear_direction_degt[0]	));	
	printf("sim/weather/shear_direction_degt[1]  	%f\n",  XPLMGetDataf( shear_direction_degt[1]	));	
	printf("sim/weather/shear_direction_degt[2]  	%f\n",  XPLMGetDataf( shear_direction_degt[2]	));	
	printf("sim/weather/shear_speed_kt[0]  		%f\n",  XPLMGetDataf( shear_speed_kt[0]		));		
	printf("sim/weather/shear_speed_kt[1]  		%f\n",  XPLMGetDataf( shear_speed_kt[1]		));		
	printf("sim/weather/shear_speed_kt[2]  		%f\n",  XPLMGetDataf( shear_speed_kt[2]		));		
	printf("sim/weather/turbulence[0]  		%f\n",  XPLMGetDataf( turbulence[0]		));			
	printf("sim/weather/turbulence[1]  		%f\n",  XPLMGetDataf( turbulence[1]		));			
	printf("sim/weather/turbulence[2]  		%f\n",  XPLMGetDataf( turbulence[2]		));			
	printf("sim/weather/wave_amplitude  		%f\n",  XPLMGetDataf( wave_amplitude		));		
	printf("sim/weather/wave_length  		%f\n",  XPLMGetDataf( wave_length		));			
	printf("sim/weather/wave_speed  		%f\n",  XPLMGetDataf( wave_speed		));			
	printf("sim/weather/wave_dir  			%d\n",  XPLMGetDatai( wave_dir			));			
	printf("sim/weather/temperature_sealevel_c  	%f\n",  XPLMGetDataf( temperature_sealevel_c	));	
	printf("sim/weather/dewpoi_sealevel_c  		%f\n",  XPLMGetDataf( dewpoi_sealevel_c		));		
	printf("sim/weather/thermal_percent  		%f\n",  XPLMGetDataf( thermal_percent		));		
	printf("sim/weather/thermal_rate_ms  		%f\n",  XPLMGetDataf( thermal_rate_ms		));		
	printf("sim/weather/thermal_altitude_msl_m  	%f\n",  XPLMGetDataf( thermal_altitude_msl_m	));	
	printf("sim/weather/runway_friction 		%f\n",  XPLMGetDataf( runway_friction		));		

	return 0;
}



// http://wrf/?lon=11.672841&lat=44.79054
int downloadData( float lon, float lat){
	char 	*command	= NULL;
	char 	*arguments[7]	= { NULL, NULL, NULL, NULL, NULL, NULL, NULL }; 
	int	str_len		= 100;
	int 	child_status;

	if ( lon <= -180 ) return 1;
	if ( lon >=  180 ) return 1;
	if ( lat <= -90  ) return 1;
	if ( lat >=  90  ) return 1;

	command 	= (char *)malloc(sizeof(char) * str_len); bzero(command, 	str_len - 1 );
	arguments[0] 	= (char *)malloc(sizeof(char) * str_len); bzero(arguments[0], 	str_len - 1 );
	arguments[1] 	= (char *)malloc(sizeof(char) * str_len); bzero(arguments[1], 	str_len - 1 );
	arguments[2] 	= (char *)malloc(sizeof(char) * str_len); bzero(arguments[2], 	str_len - 1 );
	arguments[3] 	= (char *)malloc(sizeof(char) * str_len); bzero(arguments[3], 	str_len - 1 );
	arguments[4] 	= (char *)malloc(sizeof(char) * str_len); bzero(arguments[4], 	str_len - 1 );
	arguments[5] 	= (char *)malloc(sizeof(char) * str_len); bzero(arguments[5], 	str_len - 1 );

	strcpy(command, 	"/usr/bin/wget");
	strcpy( arguments[0],	"wget");
	sprintf(arguments[1], 	"http://%s/?lon=%f&lat=%f", HOST_WRF, lon, lat);	
	strcpy( arguments[2],	"-O");
	strcpy( arguments[3],	"wrf.txt");
	strcpy( arguments[4],	"--wait=5");
	strcpy( arguments[5],	"--tries=1");


	printf("Download URL: %s\n", arguments[1]);
        pid = fork();
        if      ( pid <  0 ) return 0;
        else if ( pid == 0 ) {
                if ( execvp(command, arguments) ) {
                        printf("Unable to run wget!\n");
                        return 1;
                }
        }
	return 0;
}


int readWeatherData(){
	int i, j;
	FILE 	*fp 		= NULL;
	int 	idx 		= 0;
	float	value		= 0.0;
	char	*dataref	= NULL;
	char	*buf		= NULL;
	int	buf_size	= 200;




	static const char filename[] = "wrf.txt";

	fp = fopen(filename, "r" );
	if ( fp == NULL ){ printf("Unable to open file %s\n", filename ); return 1; }

	highLevels	= (float *)malloc( sizeof(float) * MAX_DATAREF );
	datarefs 	= (struct datarefBox *)malloc( sizeof(struct datarefBox) * MAX_DATAREF );
	for ( i = 0; i < MAX_DATAREF; i++){
		datarefs[i].idx 	= -1;
		datarefs[i].value 	= -9999;
		datarefs[i].name	= NULL;
		highLevels[i]		= -9999;

	}

	buf = (char *)malloc(sizeof(char) * buf_size);

	i = 0; j = 0;
	while(!feof(fp)){
		
		bzero(buf, buf_size - 1);
		if ( fgets(buf, buf_size, fp) == NULL ) break;
		if ( buf[0] == ';'  ) continue;
		if ( buf[0] == '\n' ) continue;
		

		datarefs[i].name = (char *)malloc(sizeof(char) * buf_size); bzero(datarefs[i].name, buf_size - 1);

		sscanf(buf,"%d %s %f\n", &(datarefs[i].idx), datarefs[i].name, &(datarefs[i].value) );
		//printf("Read: datarefs[%d].idx: %d,\tdatarefs[%d].name:  %s,\tdatarefs[%d].value:  %f\n", i, datarefs[i].idx, i, datarefs[i].name, i, datarefs[i].value );
		
		if ( ! strcmp ( datarefs[i].name, "sim/weather/wind_altitude_msl_m"     ) ) { highLevels[j] = datarefs[i].value; j++; }

		i++;
		if ( i >= MAX_DATAREF ) break;
	}

	if ( i <= 5 ) return 1;

	fclose(fp);
	return 0;
}

int MySetDataf( XPLMDataRef ref, float value) { if ( XPLMGetDataf(ref) !=      value ) 			XPLMSetDataf( ref, 	 value ); return 0; }
int MySetDatai( XPLMDataRef ref, float value) { if ( XPLMGetDatai(ref) != (int)value ) 			XPLMSetDatai( ref,  (int)value ); return 0; }



int applyDataref(float ele){
	int i, j;
	float	d		= 0;
	float 	dist_wind[3] 	= { FLT_MAX,  	FLT_MAX,	FLT_MAX };
	int	idxs_wind[3] 	= { -1,		-1, 		-1 };
	float 	dist_clod[3] 	= { FLT_MAX,  	FLT_MAX,	FLT_MAX };
	int	idxs_clod[3] 	= { -1,		-1, 		-1 };

	if ( datarefs 		== NULL ) 	return 0; 
	if ( datarefs[0].name 	== NULL ) 	return 0;


	for ( i = 0; i < MAX_DATAREF && datarefs[i].name != NULL ; i++){

		
		if ( ! strcmp ( datarefs[i].name, "sim/weather/visibility_reported_m"  	) ) { MySetDataf(visibility_reported_m,		datarefs[i].value);	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/rain_percent"  		) ) { MySetDataf(rain_percent,  		datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/thunderstorm_percent"  	) ) { MySetDataf(thunderstorm_percent,  	datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/wind_turbulence_percent" ) ) { MySetDataf(wind_turbulence_percent,  	datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/barometer_sealevel_inhg" ) ) { MySetDataf(barometer_sealevel_inhg,  	datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/use_real_weather_bool"  	) ) { MySetDataf(use_real_weather_bool,  	datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/wave_amplitude"  	) ) { MySetDataf(wave_amplitude,  		datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/wave_length"  		) ) { MySetDataf(wave_length,  			datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/wave_speed"  		) ) { MySetDataf(wave_speed,  			datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/wave_dir"  		) ) { MySetDatai(wave_dir,  			datarefs[i].value);	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/temperature_sealevel_c"  ) ) { MySetDataf(temperature_sealevel_c,  	datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/dewpoi_sealevel_c"  	) ) { MySetDataf(dewpoi_sealevel_c,  		datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/thermal_percent"  	) ) { MySetDataf(thermal_percent,  		datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/thermal_rate_ms"  	) ) { MySetDataf(thermal_rate_ms,  		datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/thermal_altitude_msl_m"  ) ) { MySetDataf(thermal_altitude_msl_m,  	datarefs[i].value); 	continue; }
		if ( ! strcmp ( datarefs[i].name, "sim/weather/runway_friction"  	) ) { MySetDataf(runway_friction,  		datarefs[i].value); 	continue; }


		if ( ! strcmp ( datarefs[i].name, "sim/weather/wind_altitude_msl_m" 	) ) {
			d = fabs( ele - datarefs[i].value );
			for ( j = 0; j < 3; j++) if ( d < dist_wind[j] ) { dist_wind[j] = d; idxs_wind[j] = datarefs[i].idx; break; }
			continue;
		}
		
		if ( ! strcmp ( datarefs[i].name, "sim/weather/cloud_base_msl_m" 	) ) { 
			d = fabs( ele - datarefs[i].value );
			for ( j = 0; j < 3; j++) if ( d < dist_clod[j] ) { dist_clod[j] = d; idxs_clod[j] = datarefs[i].idx; break; }
			continue;
		}

	}



	for ( j = 0; j < 3; j++) {

//		printf("idxs_wind[%d]: %d, idxs_clod[%d]: %d, dist_wind[%d]: %f, dist_clod[%d]: %f\n", j, idxs_wind[j], j, idxs_clod[j], j, dist_wind[j], j, dist_clod[j] );

		for ( i = 0; i < MAX_DATAREF && datarefs[i].name != NULL ; i++){
			if ( datarefs[i].idx < 0 ) continue;

			if ( idxs_wind[j] != -1 ){
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/wind_altitude_msl_m"   ) ) && ( datarefs[i].idx == idxs_wind[j] ) ) { MySetDataf(wind_altitude_msl_m[j], 	datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/wind_direction_degt"   ) ) && ( datarefs[i].idx == idxs_wind[j] ) ) { MySetDataf(wind_direction_degt[j], 	datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/wind_speed_kt"    	) ) && ( datarefs[i].idx == idxs_wind[j] ) ) { MySetDataf(wind_speed_kt[j], 		datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/shear_direction_degt" 	) ) && ( datarefs[i].idx == idxs_wind[j] ) ) { MySetDataf(shear_direction_degt[j], 	datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/shear_speed_kt"    	) ) && ( datarefs[i].idx == idxs_wind[j] ) ) { MySetDataf(shear_speed_kt[j], 		datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/turbulence"    	) ) && ( datarefs[i].idx == idxs_wind[j] ) ) { MySetDataf(turbulence[j], 		datarefs[i].value); continue; }
			}
			if ( idxs_clod[j] != -1 ){
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/cloud_coverage"    	) ) && ( datarefs[i].idx == idxs_clod[j] ) ) { MySetDataf(cloud_coverage[j], 		datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/cloud_type"    	) ) && ( datarefs[i].idx == idxs_clod[j] ) ) { MySetDatai(cloud_type[j], 		datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/cloud_base_msl_m"   	) ) && ( datarefs[i].idx == idxs_clod[j] ) ) { MySetDataf(cloud_base_msl_m[j], 		datarefs[i].value); continue; }
	if ( ( ! strcmp ( datarefs[i].name, "sim/weather/cloud_tops_msl_m"   	) ) && ( datarefs[i].idx == idxs_clod[j] ) ) { MySetDataf(cloud_tops_msl_m[j], 		datarefs[i].value); continue; }
			}

		}

		
		if ( idxs_clod[j] == -1 ) MySetDatai(cloud_type[j], 		0);
	

	}
	

	return 0;
}


int getWRFlevel( float ele){
	int i;
	for ( i = 0; i < MAX_DATAREF && highLevels[i] != -9999 ; i++){
		if ( highLevels[i] > ele ) break;
	}
	return i;
}



float   MyFlightLoopCallback(
                                   float                inElapsedSinceLastCall,
                                   float                inElapsedTimeSinceLastFlightLoop,
                                   int                  inCounter,
                                   void *               inRefcon){

	float 	lat 	= XPLMGetDataf(latitude);
	float 	lon 	= XPLMGetDataf(longitude);
	float 	ele 	= XPLMGetDataf(elevation);
	float	dist	= 0;
	int 	child_status;

	MySetDatai(use_real_weather_bool, 0 );

	printf("STATUS: %d\n", STATUS);

	if ( STATUS == FIRST_START ){
		printf("First run, Init variables ...\n");
		latOld 	= XPLMGetDataf(latitude);
		lonOld 	= XPLMGetDataf(longitude);
		eleOld 	= XPLMGetDataf(elevation);
		STATUS  = READY_TO_DOWNLOAD;
		return 1.0;
	}

	if (  STATUS == READY_TO_DOWNLOAD ){
		downloadData(lon, lat);
		STATUS = DOWNLOADING;
		return 1.0;
	}

	if ( STATUS == DOWNLOADING ){
		if ( waitpid(pid, &child_status, WNOHANG) != -1 ) return 1.0;
		STATUS = READY_TO_READ;
		return 1.0;
	}

	if ( STATUS == READY_TO_READ ){
		if ( readWeatherData() == 1 ) 	STATUS = READY_TO_DOWNLOAD;
		else				STATUS = READY_TO_APPLY;
		return 1.0;

	}

	if ( STATUS == READY_TO_APPLY ){
	        applyDataref(ele);
		STATUS = WAITING_FOR_NEWS;
		return 1.0;

	}

	if ( STATUS != WAITING_FOR_NEWS ) return 1.0;

	dist = sqrt( ((lon - lonOld)*(lon - lonOld)) + ((lat-latOld)*(lat-latOld)) );
	if ( dist > WRF_RESOLUTION ){
		STATUS = READY_TO_DOWNLOAD;
		latOld = lat; 
		lonOld = lon;
		return 1.0;
	}


	if ( eleWRF != getWRFlevel(ele) ){
		STATUS = READY_TO_APPLY;
		eleWRF = getWRFlevel(ele);
		return 1.0;

	}

	printWeatherParams();



	eleOld = ele;

        return 2.0;

}



PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc){

	strcpy(outName, "SimData");
	strcpy(outSig, "xplanesdk.examples.simdata");
	strcpy(outDesc, "A plugin that changes sim data.");

	use_real_weather_bool 	= XPLMFindDataRef("sim/weather/use_real_weather_bool"); 	if ( use_real_weather_bool 	== NULL ) return 0; 

	cloud_type[0]		= XPLMFindDataRef("sim/weather/cloud_type[0]"); 		if ( cloud_type[0] 		== NULL ) return 0; 
	cloud_type[1]		= XPLMFindDataRef("sim/weather/cloud_type[1]"); 		if ( cloud_type[1] 		== NULL ) return 0; 
	cloud_type[2]		= XPLMFindDataRef("sim/weather/cloud_type[2]"); 		if ( cloud_type[2] 		== NULL ) return 0; 

	cloud_coverage[0]	= XPLMFindDataRef("sim/weather/cloud_coverage[0]"); 		if ( cloud_coverage[0] 		== NULL ) return 0; 
	cloud_coverage[1]	= XPLMFindDataRef("sim/weather/cloud_coverage[1]"); 		if ( cloud_coverage[1] 		== NULL ) return 0; 
	cloud_coverage[2]	= XPLMFindDataRef("sim/weather/cloud_coverage[2]"); 		if ( cloud_coverage[2] 		== NULL ) return 0; 


	cloud_base_msl_m[0]	= XPLMFindDataRef("sim/weather/cloud_base_msl_m[0]"); 		if ( cloud_base_msl_m[0] 	== NULL ) return 0; 
	cloud_base_msl_m[1]	= XPLMFindDataRef("sim/weather/cloud_base_msl_m[1]"); 		if ( cloud_base_msl_m[1] 	== NULL ) return 0; 
	cloud_base_msl_m[2]	= XPLMFindDataRef("sim/weather/cloud_base_msl_m[2]"); 		if ( cloud_base_msl_m[2] 	== NULL ) return 0; 

	cloud_tops_msl_m[0]	= XPLMFindDataRef("sim/weather/cloud_tops_msl_m[0]"); 		if ( cloud_tops_msl_m[0] 	== NULL ) return 0; 
	cloud_tops_msl_m[1]	= XPLMFindDataRef("sim/weather/cloud_tops_msl_m[1]"); 		if ( cloud_tops_msl_m[1] 	== NULL ) return 0; 
	cloud_tops_msl_m[2]	= XPLMFindDataRef("sim/weather/cloud_tops_msl_m[2]"); 		if ( cloud_tops_msl_m[2] 	== NULL ) return 0; 


	visibility_reported_m	= XPLMFindDataRef("sim/weather/visibility_reported_m");		if ( visibility_reported_m 	== NULL ) return 0; 
	rain_percent		= XPLMFindDataRef("sim/weather/rain_percent");			if ( rain_percent 		== NULL ) return 0; 
	thunderstorm_percent	= XPLMFindDataRef("sim/weather/thunderstorm_percent");		if ( thunderstorm_percent	== NULL ) return 0;
	wind_turbulence_percent	= XPLMFindDataRef("sim/weather/wind_turbulence_percent"); 	if ( wind_turbulence_percent 	== NULL ) return 0;
	barometer_sealevel_inhg = XPLMFindDataRef("sim/weather/barometer_sealevel_inhg");	if ( barometer_sealevel_inhg	== NULL ) return 0;
	

	wind_altitude_msl_m[0]	= XPLMFindDataRef("sim/weather/wind_altitude_msl_m[0]");	if ( wind_altitude_msl_m[0]	== NULL ) return 0;
	wind_altitude_msl_m[1]	= XPLMFindDataRef("sim/weather/wind_altitude_msl_m[1]");	if ( wind_altitude_msl_m[1]	== NULL ) return 0;
	wind_altitude_msl_m[2]	= XPLMFindDataRef("sim/weather/wind_altitude_msl_m[2]");	if ( wind_altitude_msl_m[2]	== NULL ) return 0;

	wind_direction_degt[0]	= XPLMFindDataRef("sim/weather/wind_direction_degt[0]");	if ( wind_direction_degt[0]	== NULL ) return 0;
	wind_direction_degt[1]	= XPLMFindDataRef("sim/weather/wind_direction_degt[1]");	if ( wind_direction_degt[1]	== NULL ) return 0;
	wind_direction_degt[2]	= XPLMFindDataRef("sim/weather/wind_direction_degt[2]");	if ( wind_direction_degt[2]	== NULL ) return 0;

	wind_speed_kt[0]	= XPLMFindDataRef("sim/weather/wind_speed_kt[0]");		if ( wind_speed_kt[0]		== NULL ) return 0;
	wind_speed_kt[1]	= XPLMFindDataRef("sim/weather/wind_speed_kt[1]");		if ( wind_speed_kt[1]		== NULL ) return 0;
	wind_speed_kt[2]	= XPLMFindDataRef("sim/weather/wind_speed_kt[2]");		if ( wind_speed_kt[2]		== NULL ) return 0;

	shear_direction_degt[0]	= XPLMFindDataRef("sim/weather/shear_direction_degt[0]");	if ( shear_direction_degt[0]	== NULL ) return 0;
	shear_direction_degt[1]	= XPLMFindDataRef("sim/weather/shear_direction_degt[1]");	if ( shear_direction_degt[1]	== NULL ) return 0;
	shear_direction_degt[2]	= XPLMFindDataRef("sim/weather/shear_direction_degt[2]");	if ( shear_direction_degt[2]	== NULL ) return 0;

	shear_speed_kt[0]	= XPLMFindDataRef("sim/weather/shear_speed_kt[0]");		if ( shear_speed_kt[0]		== NULL ) return 0;
	shear_speed_kt[1]	= XPLMFindDataRef("sim/weather/shear_speed_kt[1]");		if ( shear_speed_kt[1]		== NULL ) return 0;
	shear_speed_kt[2]	= XPLMFindDataRef("sim/weather/shear_speed_kt[2]");		if ( shear_speed_kt[2]		== NULL ) return 0;

	turbulence[0]		= XPLMFindDataRef("sim/weather/turbulence[0]");			if ( turbulence[0]		== NULL ) return 0;
	turbulence[1]		= XPLMFindDataRef("sim/weather/turbulence[1]");			if ( turbulence[1]		== NULL ) return 0;
	turbulence[2]		= XPLMFindDataRef("sim/weather/turbulence[2]");			if ( turbulence[2]		== NULL ) return 0;


	wave_amplitude		= XPLMFindDataRef("sim/weather/wave_amplitude");		if ( wave_amplitude		== NULL ) return 0;
	wave_length		= XPLMFindDataRef("sim/weather/wave_length");			if ( wave_length		== NULL ) return 0;
	wave_speed		= XPLMFindDataRef("sim/weather/wave_speed");			if ( wave_speed			== NULL ) return 0;
	wave_dir		= XPLMFindDataRef("sim/weather/wave_dir");			if ( wave_dir			== NULL ) return 0;
	temperature_sealevel_c	= XPLMFindDataRef("sim/weather/temperature_sealevel_c");	if ( temperature_sealevel_c	== NULL ) return 0;
	dewpoi_sealevel_c	= XPLMFindDataRef("sim/weather/dewpoi_sealevel_c");		if ( dewpoi_sealevel_c		== NULL ) return 0;
	thermal_percent		= XPLMFindDataRef("sim/weather/thermal_percent");		if ( thermal_percent		== NULL ) return 0;
	thermal_rate_ms		= XPLMFindDataRef("sim/weather/thermal_rate_ms");		if ( thermal_rate_ms		== NULL ) return 0;
	thermal_altitude_msl_m	= XPLMFindDataRef("sim/weather/thermal_altitude_msl_m");	if ( thermal_altitude_msl_m	== NULL ) return 0;
	runway_friction		= XPLMFindDataRef("sim/weather/runway_friction");		if ( runway_friction		== NULL ) return 0;


	latitude		= XPLMFindDataRef("sim/flightmodel/position/latitude");		if ( latitude			== NULL ) return 0;
	longitude		= XPLMFindDataRef("sim/flightmodel/position/longitude");	if ( longitude			== NULL ) return 0;
	elevation		= XPLMFindDataRef("sim/flightmodel/position/elevation");	if ( elevation			== NULL ) return 0;



	unlink("wrf.txt");
	XPLMRegisterFlightLoopCallback( MyFlightLoopCallback, 1.0, NULL);                               

	
	return 1;
}

PLUGIN_API void	XPluginStop(void)	{}
PLUGIN_API void XPluginDisable(void)	{}
PLUGIN_API int XPluginEnable(void)	{ return 1; }
PLUGIN_API void XPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					long			inMessage,
					void *			inParam){}

                                   
