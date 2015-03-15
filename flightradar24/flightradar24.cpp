#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "XPLMPlanes.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMGraphics.h"
#include "XPLMDefs.h"
#include "XPLMScenery.h"



#ifdef __arch64__
#define CURL_SIZEOF_LONG 8
#endif 

#define FREE			0
#define	USED			1
#define OLD			2
#define NEW			3
#define FALSE			0
#define	TRUE			1
#define CODE_LENGHT		128
#define MAX_AIR_CRAFT_NUM 	19
#define MAX_AIR_CRAFT_DIST	50 // Km


XPLMDataRef	gPlaneLat;
XPLMDataRef 	gPlaneLon;
XPLMDataRef 	gPlaneEl;
XPLMDataRef	gPlaneX;
XPLMDataRef	gPlaneY;
XPLMDataRef	gPlaneZ;
XPLMDataRef	gPlaneTheta;
XPLMDataRef	gPlanePhi;
XPLMDataRef	gPlanePsi;
XPLMDataRef	gOverRidePlanePosition;
XPLMDataRef	gAGL;
XPLMProbeRef	myProbe;

struct MemoryStruct {
        char *memory;
        size_t size;
};


void *getDataFromFlightRadar24(void *arg);


class Aircraft {
	private:
		XPLMDataRef	dr_plane_x;
		XPLMDataRef	dr_plane_y;
		XPLMDataRef	dr_plane_z;
		XPLMDataRef	dr_plane_the;
		XPLMDataRef	dr_plane_phi;
		XPLMDataRef	dr_plane_psi;
		XPLMDataRef	dr_plane_gear_deploy;
		XPLMDataRef	dr_plane_throttle;
		XPLMDataRef	dr_plane_el;
	public:
		float		plane_x;
		float		plane_y;
		float		plane_z;
		float		plane_the;
		float		plane_phi;
		float		plane_psi;
		float		plane_gear_deploy[5];
		float		plane_throttle[8];
		float		plane_el;

		Aircraft(int AircraftNo);
		void GetAircraftData(void);
		void SetAircraftData(void);
};

Aircraft::Aircraft(int AircraftNo){
	char	x_str[80];
	char	y_str[80];
	char	z_str[80];
	char	the_str[80];
	char	phi_str[80];
	char	psi_str[80];
	char	gear_deploy_str[80];
	char	throttle_str[80];
	char	el_str[80];

	strcpy(x_str, 		"sim/multiplayer/position/planeX_x");
	strcpy(y_str,		"sim/multiplayer/position/planeX_y");
	strcpy(z_str,		"sim/multiplayer/position/planeX_z");
	strcpy(the_str,		"sim/multiplayer/position/planeX_the");
	strcpy(phi_str,		"sim/multiplayer/position/planeX_phi");
	strcpy(psi_str,		"sim/multiplayer/position/planeX_psi");
	strcpy(gear_deploy_str,	"sim/multiplayer/position/planeX_gear_deploy");
	strcpy(throttle_str, 	"sim/multiplayer/position/planeX_throttle");
	strcpy(el_str,		"sim/multiplayer/position/planeX_el");


	char cTemp = (AircraftNo + 0x30);
	x_str[30]		=	cTemp;
	y_str[30]		=	cTemp;
	z_str[30]		=	cTemp;
	the_str[30]		=	cTemp;
	phi_str[30]		=	cTemp;
	psi_str[30]		=	cTemp;
	gear_deploy_str[30] 	=	cTemp;
	throttle_str[30]	=	cTemp;
	el_str[30]		=	cTemp;

	dr_plane_x		= XPLMFindDataRef(x_str);
	dr_plane_y		= XPLMFindDataRef(y_str);
	dr_plane_z		= XPLMFindDataRef(z_str);
	dr_plane_the		= XPLMFindDataRef(the_str);
	dr_plane_phi		= XPLMFindDataRef(phi_str);
	dr_plane_psi		= XPLMFindDataRef(psi_str);
	dr_plane_gear_deploy	= XPLMFindDataRef(gear_deploy_str);
	dr_plane_throttle	= XPLMFindDataRef(throttle_str);
	dr_plane_el		= XPLMFindDataRef(el_str);

}

void Aircraft::GetAircraftData(void){
	plane_x		= XPLMGetDataf(dr_plane_x);
	plane_y		= XPLMGetDataf(dr_plane_y);
	plane_z		= XPLMGetDataf(dr_plane_z);
	plane_the	= XPLMGetDataf(dr_plane_the);
	plane_phi	= XPLMGetDataf(dr_plane_phi);
	plane_psi	= XPLMGetDataf(dr_plane_psi);
	plane_el	= XPLMGetDataf(dr_plane_el);

	XPLMGetDatavf(dr_plane_gear_deploy, plane_gear_deploy, 0, 5);
	XPLMGetDatavf(dr_plane_throttle, plane_throttle, 0, 8);
}

void Aircraft::SetAircraftData(void){
	XPLMSetDataf(dr_plane_x, 	plane_x);
	XPLMSetDataf(dr_plane_y, 	plane_y);
	XPLMSetDataf(dr_plane_z, 	plane_z);
	XPLMSetDataf(dr_plane_the, 	plane_the);
	XPLMSetDataf(dr_plane_phi, 	plane_phi);
	XPLMSetDataf(dr_plane_psi, 	plane_psi);


	XPLMSetDatavf(dr_plane_gear_deploy, plane_gear_deploy, 0, 5);
	XPLMSetDatavf(dr_plane_throttle, plane_throttle, 0, 8);
}


struct AircraftData{
	double		lat;
	double		lon;
	double		course;
	double		ele;
	double		speed;
	double		vspeed;
	double		itime;
	int		status;
	int		age;
	char		*name;
	int		time;
	Aircraft	*obj;
	struct AircraftData *next;
	int		index;
};

struct AirplaneData{
	double		x;
	double		y;
	double		z;
	double		theta;
	double		phi;
	double		psi;
        double   	lat; 
        double   	lon;
        double   	ele;
        double   	alt;
	double		elapsed;

};


struct  AircraftData *AircraftDataArray;
struct  AirplaneData myPlaneInfo;
int	DownloadStatus;

// -------------------------------------------------------------------------------------------------- //

double	coordsZone[32][4];
char 	namesZone[32][20]={ "europe", "poland", "germany", "uk", "london", "ireland", "spain", "france", "ceur", "scandinavia", "italy", "northamerica", "na_n", "na_c", "na_cny", "na_cla", "na_cat", "na_cse", "na_nw", "na_ne", "na_sw", "na_se", "na_cc", "na_s", "southamerica", "oceania", "asia", "japan", "africa", "atlantic", "maldives", "northatlantic" };


           
// -------------------------------------------------------------------------------------------------- //



// Aircraft Aircraft1(1);

static float	MyFlightLoopCallback0(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon);    

static float	MyFlightLoopCallback1(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon);    

static float	MyFlightLoopCallback(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon);    


PLUGIN_API int XPluginStart(	char *		outName,
				char *		outSig,
				char *		outDesc){

	strcpy(outName, "Flightradar24");
	strcpy(outSig, "xplanesdk.examples.drawaircraft");
	strcpy(outDesc, "A plugin that draws aircraft.");

	/* Prefetch the sim variables we will use. */
        gPlaneLat		= XPLMFindDataRef("sim/flightmodel/position/latitude");
        gPlaneLon		= XPLMFindDataRef("sim/flightmodel/position/longitude");
        gPlaneEl		= XPLMFindDataRef("sim/flightmodel/position/elevation");
	gPlaneX			= XPLMFindDataRef("sim/flightmodel/position/local_x");
	gPlaneY			= XPLMFindDataRef("sim/flightmodel/position/local_y");
	gPlaneZ			= XPLMFindDataRef("sim/flightmodel/position/local_z");
	gPlaneTheta		= XPLMFindDataRef("sim/flightmodel/position/theta");
	gPlanePhi		= XPLMFindDataRef("sim/flightmodel/position/phi");
	gPlanePsi		= XPLMFindDataRef("sim/flightmodel/position/psi");
	gOverRidePlanePosition 	= XPLMFindDataRef("sim/operation/override/override_planepath");
	gAGL 			= XPLMFindDataRef("sim/flightmodel/position/y_agl");

	XPLMRegisterFlightLoopCallback(		
			MyFlightLoopCallback,	/* Callback */
			1.0,			/* Interval */
			NULL);			/* refcon not used. */


	XPLMRegisterFlightLoopCallback(		
			MyFlightLoopCallback0,	/* Callback */
			1.0,			/* Interval */
			NULL);			/* refcon not used. */



	XPLMRegisterFlightLoopCallback(		
			MyFlightLoopCallback1,	/* Callback */
			1.0,			/* Interval */
			NULL);			/* refcon not used. */

	myProbe = XPLMCreateProbe(xplm_ProbeY);

	AircraftDataArray = (struct AircraftData *)malloc(sizeof( struct AircraftData ) * MAX_AIR_CRAFT_NUM );
	if ( AircraftDataArray == NULL ) return 0;
	for ( int i = 1; i < MAX_AIR_CRAFT_NUM; i++ ) {
		AircraftDataArray[i].status 	= FREE;
		AircraftDataArray[i].name	= (char *)malloc(sizeof(char) * CODE_LENGHT); bzero(AircraftDataArray[i].name, CODE_LENGHT - 1);
		AircraftDataArray[i].obj	= NULL;
		AircraftDataArray[i].index	= -1;
	}

	myPlaneInfo.x		= 0;
	myPlaneInfo.y		= 0;
	myPlaneInfo.z		= 0;
	myPlaneInfo.theta	= 0;
	myPlaneInfo.phi		= 0;
	myPlaneInfo.psi		= 0;
	myPlaneInfo.lat     	= 0;
	myPlaneInfo.lon     	= 0;
	myPlaneInfo.ele      	= 0;
	myPlaneInfo.alt		= 0;
	myPlaneInfo.elapsed 	= 0;	
	DownloadStatus		= TRUE;

	coordsZone[0 ][0]=72.570000;	coordsZone[0 ][1]=-16.960000;	coordsZone[0 ][2]=33.570000;	coordsZone[0 ][3]=53.050000;
	coordsZone[1 ][0]=56.860000;	coordsZone[1 ][1]=11.060000;	coordsZone[1 ][2]=48.220000;	coordsZone[1 ][3]=28.260000;
	coordsZone[2 ][0]=57.920000;	coordsZone[2 ][1]=1.810000;	coordsZone[2 ][2]=45.810000;	coordsZone[2 ][3]=16.830000;
	coordsZone[3 ][0]=62.610000;	coordsZone[3 ][1]=-13.070000;	coordsZone[3 ][2]=49.710000;	coordsZone[3 ][3]=3.460000;
	coordsZone[4 ][0]=53.060000;	coordsZone[4 ][1]=-2.870000;	coordsZone[4 ][2]=50.070000;	coordsZone[4 ][3]=3.260000;
	coordsZone[5 ][0]=56.220000;	coordsZone[5 ][1]=-11.710000;	coordsZone[5 ][2]=50.910000;	coordsZone[5 ][3]=-4.400000;
	coordsZone[6 ][0]=44.360000;	coordsZone[6 ][1]=-11.060000;	coordsZone[6 ][2]=35.760000;	coordsZone[6 ][3]=4.040000;
	coordsZone[7 ][0]=51.070000;	coordsZone[7 ][1]=-5.180000;	coordsZone[7 ][2]=42.170000;	coordsZone[7 ][3]=8.900000;
	coordsZone[8 ][0]=51.390000;	coordsZone[8 ][1]=11.250000;	coordsZone[8 ][2]=39.720000;	coordsZone[8 ][3]=32.550000;
	coordsZone[9 ][0]=72.120000;	coordsZone[9 ][1]=-0.730000;	coordsZone[9 ][2]=53.820000;	coordsZone[9 ][3]=40.670000;
	coordsZone[10][0]=47.670000;	coordsZone[10][1]=5.260000;	coordsZone[10][2]=36.270000;	coordsZone[10][3]=20.640000;
	coordsZone[11][0]=75.000000;	coordsZone[11][1]=-180.000000;	coordsZone[11][2]=3.000000;	coordsZone[11][3]=-52.000000;
	coordsZone[12][0]=72.820000;	coordsZone[12][1]=-177.970000;	coordsZone[12][2]=41.920000;	coordsZone[12][3]=-52.480000;
	coordsZone[13][0]=54.660000;	coordsZone[13][1]=-134.680000;	coordsZone[13][2]=22.160000;	coordsZone[13][3]=-56.910000;
	coordsZone[14][0]=45.060000;	coordsZone[14][1]=-83.690000;	coordsZone[14][2]=35.960000;	coordsZone[14][3]=-64.290000;
	coordsZone[15][0]=37.910000;	coordsZone[15][1]=-126.120000;	coordsZone[15][2]=30.210000;	coordsZone[15][3]=-110.020000;
	coordsZone[16][0]=35.860000;	coordsZone[16][1]=-92.610000;	coordsZone[16][2]=22.560000;	coordsZone[16][3]=-71.190000;
	coordsZone[17][0]=49.120000;	coordsZone[17][1]=-126.150000;	coordsZone[17][2]=42.970000;	coordsZone[17][3]=-111.920000;
	coordsZone[18][0]=54.120000;	coordsZone[18][1]=-134.130000;	coordsZone[18][2]=38.320000;	coordsZone[18][3]=-96.750000;
	coordsZone[19][0]=53.720000;	coordsZone[19][1]=-98.760000;	coordsZone[19][2]=38.220000;	coordsZone[19][3]=-57.360000;
	coordsZone[20][0]=38.920000;	coordsZone[20][1]=-133.980000;	coordsZone[20][2]=22.620000;	coordsZone[20][3]=-96.750000;
	coordsZone[21][0]=38.520000;	coordsZone[21][1]=-98.620000;	coordsZone[21][2]=22.520000;	coordsZone[21][3]=-57.360000;
	coordsZone[22][0]=45.920000;	coordsZone[22][1]=-116.880000;	coordsZone[22][2]=27.620000;	coordsZone[22][3]=-75.910000;
	coordsZone[23][0]=41.920000;	coordsZone[23][1]=-177.830000;	coordsZone[23][2]=3.820000;	coordsZone[23][3]=-52.480000;
	coordsZone[24][0]=16.000000;	coordsZone[24][1]=-96.000000;	coordsZone[24][2]=-57.000000;	coordsZone[24][3]=-31.000000;
	coordsZone[25][0]=19.620000;	coordsZone[25][1]=88.400000;	coordsZone[25][2]=-55.080000;	coordsZone[25][3]=180.000000;
	coordsZone[26][0]=79.980000;	coordsZone[26][1]=40.910000;	coordsZone[26][2]=12.480000;	coordsZone[26][3]=179.770000;
	coordsZone[27][0]=60.380000;	coordsZone[27][1]=113.500000;	coordsZone[27][2]=22.580000;	coordsZone[27][3]=176.470000;
	coordsZone[28][0]=39.000000;	coordsZone[28][1]=-29.000000;	coordsZone[28][2]=-39.000000;	coordsZone[28][3]=55.000000;
	coordsZone[29][0]=52.620000;	coordsZone[29][1]=-50.900000;	coordsZone[29][2]=15.620000;	coordsZone[29][3]=-4.750000;
	coordsZone[30][0]=10.720000;	coordsZone[30][1]=63.100000;	coordsZone[30][2]=-6.080000;	coordsZone[30][3]=86.530000;
	coordsZone[31][0]=82.620000;	coordsZone[31][1]=-84.530000;	coordsZone[31][2]=59.020000;	coordsZone[31][3]=4.450000;


	return 1;
}


PLUGIN_API void	XPluginStop(void){
	XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback0, NULL);
	XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback1, NULL);
	XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback, NULL);
}

PLUGIN_API void XPluginDisable(void){}
PLUGIN_API int XPluginEnable(void){ return 1; }

PLUGIN_API void XPluginReceiveMessage(	XPLMPluginID	inFromWho,
					int		inMessage,
					void 		*inParam){}

// -------------------------------------------------------------------------------------------------- //



static void *myrealloc(void *ptr, size_t size){
        if(ptr) return realloc(ptr, size);
        else    return malloc(size);
}


static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data){
        size_t realsize                 = size * nmemb;
        struct MemoryStruct *mem        = (struct MemoryStruct *)data;

        mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
        if (mem->memory) {
                memcpy(&(mem->memory[mem->size]), ptr, realsize);
                mem->size += realsize;
                mem->memory[mem->size] = 0;
        }
        return realsize;
}


float distAprox(float lon1, float lat1, float lon2, float lat2) {
        float  R       = 6372.795477598;
        float  dLat    = 0.0;
        float  dLon    = 0.0;
        float  a       = 0.0;
        float  c       = 0.0;

        dLat = (lat2-lat1) * ( M_PI / 180.0 );
        dLon = (lon2-lon1) * ( M_PI / 180.0 );

        a       = sin(dLat/2.0) * sin(dLat/2.0) + cos(lat1 * ( M_PI / 180.0 )) * cos(lat2 * ( M_PI / 180.0 )) * sin(dLon/2.0) * sin(dLon/2.0);
        c       = 2.0 * atan2(sqrt(a), sqrt(1-a));

        return( R * c);
}

float	MyFlightLoopCallback0(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon){

	// Disable AI for each aircraft.
	// for (int AircraftIndex = 1; AircraftIndex < MAX_AIR_CRAFT_NUM ; AircraftIndex++) XPLMDisableAIForPlane(AircraftIndex);
	
	int 	active 			= 1;
	int	outTotalAircraft	= 0;
	int	outActiveAircraft	= 0;    
	XPLMPluginID	*outController		= NULL;	

	for ( int k = 1; k < MAX_AIR_CRAFT_NUM; k++){ if (  AircraftDataArray[k].index != -1 ) active++; }

	XPLMCountAircraft(&outTotalAircraft, &outActiveAircraft, outController); 
	if ( active <  outActiveAircraft ) XPLMReleasePlanes();

	return 10.0;
}

float	MyFlightLoopCallback1(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon){

	pthread_t thread;

	if ( myPlaneInfo.elapsed == 0 	) return 1.0;
	if ( DownloadStatus != TRUE	) return 1.0;

	DownloadStatus = FALSE;
	pthread_create(&thread, NULL, getDataFromFlightRadar24, (void *)NULL);

	return 10.0;
}




float	MyFlightLoopCallback(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon){

	int	GearState, k, i, j;
	double	outX, outY, outZ;
	double	tmp;	
	Aircraft 	*aircraft 		= NULL;
	int		outTotalAircraft	= 0;
	int		outActiveAircraft	= 0;    
	XPLMPluginID	*outController		= NULL;	
	float		delay 			= 0.05;
	double		elapsed			= 0;

	// Get User Aircraft data
	myPlaneInfo.x		= XPLMGetDataf(gPlaneX);
	myPlaneInfo.y		= XPLMGetDataf(gPlaneY);
	myPlaneInfo.z		= XPLMGetDataf(gPlaneZ);
	myPlaneInfo.theta	= XPLMGetDataf(gPlaneTheta);
	myPlaneInfo.phi		= XPLMGetDataf(gPlanePhi);
	myPlaneInfo.psi		= XPLMGetDataf(gPlanePsi);
	myPlaneInfo.lat     	= XPLMGetDataf(gPlaneLat);
	myPlaneInfo.lon     	= XPLMGetDataf(gPlaneLon);
	myPlaneInfo.ele      	= XPLMGetDataf(gPlaneEl);
	myPlaneInfo.alt		= XPLMGetDataf(gAGL);
	elapsed			= XPLMGetElapsedTime();
	elapsed			= elapsed - myPlaneInfo.elapsed;
	myPlaneInfo.elapsed 	= XPLMGetElapsedTime();
	XPLMProbeInfo_t  outInfo;
	XPLMProbeResult  result;

	outInfo.structSize = sizeof(outInfo);

	XPLMCountAircraft(&outTotalAircraft, &outActiveAircraft, outController); 

	for (  i = 1; i != outActiveAircraft; i++ ) {
		for ( k = 1; k < MAX_AIR_CRAFT_NUM; k++){ if ( ( AircraftDataArray[k].status == USED ) && ( AircraftDataArray[k].index == i ) ) break; }

		if ( k == MAX_AIR_CRAFT_NUM ) {
			for ( k = 1; k < MAX_AIR_CRAFT_NUM; k++ ){ if ( ( AircraftDataArray[k].status == USED ) && ( AircraftDataArray[k].index == -1 ) ) break; }
			if  ( k == MAX_AIR_CRAFT_NUM ) continue;

			if ( AircraftDataArray[k].obj != NULL ){
				delete AircraftDataArray[k].obj;
				AircraftDataArray[k].obj = NULL;
			}

			printf("New registration airplane %d ... \n", i );

			AircraftDataArray[k].obj 	= new Aircraft(i); 
			AircraftDataArray[k].index 	= i;
		} 

		
		aircraft = AircraftDataArray[k].obj;
		aircraft->GetAircraftData();


		if ( AircraftDataArray[k].age == NEW ){
			XPLMWorldToLocal( AircraftDataArray[k].lat, AircraftDataArray[k].lon, AircraftDataArray[k].ele, &outX, &outY, &outZ );
		} else {
		
			outX = aircraft->plane_x + cos( fmod( AircraftDataArray[k].course - 90, 360 ) * M_PI / 180.0  ) * AircraftDataArray[k].speed * elapsed;
			outZ = aircraft->plane_z + sin( fmod( AircraftDataArray[k].course - 90, 360 ) * M_PI / 180.0  ) * AircraftDataArray[k].speed * elapsed;
			outY = aircraft->plane_y + AircraftDataArray[k].vspeed * elapsed;
		}




		result = XPLMProbeTerrainXYZ(myProbe, outX, outY, outZ, &outInfo);
		if ( outY < outInfo.locationY ) outY = outInfo.locationY;	

		aircraft->plane_x 	= outX; 
		aircraft->plane_y 	= outY;  
		aircraft->plane_z 	= outZ;

		aircraft->plane_the	= 0.0;
		aircraft->plane_phi	= 0.0;
		aircraft->plane_psi	= AircraftDataArray[k].course;

	
		if (AircraftDataArray[k].ele > 200 ) 	GearState = 0;
		else					GearState = 1;
		for (int Gear = 0; Gear < 5; Gear++) aircraft->plane_gear_deploy[Gear]	= GearState;
		for (int Thro = 0; Thro < 8; Thro++) aircraft->plane_throttle[Thro]	= 0.6;

		AircraftDataArray[k].age 	= OLD;
		AircraftDataArray[k].itime 	= myPlaneInfo.elapsed;
		XPLMLocalToWorld(outX, outY, outZ, &(AircraftDataArray[k].lat), &(AircraftDataArray[k].lon), &(AircraftDataArray[k].ele));
		aircraft->SetAircraftData();
		
	}

	


	return delay;
}


// ------------------------------------------------------------------------------- //

void *getDataFromFlightRadar24(void *arg){
	struct  	MemoryStruct chunk;
	CURL    	*curl_handle;
	CURLcode 	res;
	char		*token	= NULL;
	int		i 	= 0;
	int		cnt	= 0;
	int		j 	= 0;
	int		k 	= 0;
	int		valid	= 0;
	int		minTime	= INT_MAX;
        float   	lat     = 0; 
        float   	lon     = 0;
        float   	el      = 0;
	float		dist	= 0;
	char		*plane_code	= NULL;
	float		plane_lat	= 0;
	float		plane_lon	= 0;
	float		plane_ele	= 0;
	float		plane_course	= 0;
	float		plane_speed	= 0;
	int		plane_time	= 0;
	double		area		= 0;
	double		area_old	= 0;
	char		url[255];
	struct AircraftData *head	= NULL;
	struct AircraftData *cursor	= NULL;
	struct AircraftData *tmp	= NULL; // User for bubble sort
	struct AircraftData *a		= NULL; 
	struct AircraftData *b		= NULL;
	struct AircraftData *c		= NULL;
	struct AircraftData *d		= NULL;
	struct AircraftData *e		= NULL;



	chunk.memory    = NULL;
        chunk.size      = 0;
	curl_handle	= curl_easy_init();

	for ( i = 0, j = -1; i < 32 ; i++ ){
		if ( coordsZone[i][0] < myPlaneInfo.lat ) continue;
		if ( coordsZone[i][2] > myPlaneInfo.lat ) continue;		
		if ( coordsZone[i][1] > myPlaneInfo.lon ) continue;
		if ( coordsZone[i][3] < myPlaneInfo.lon ) continue;
	

		if ( j < 0 ) 	area_old = ( coordsZone[i][0] - coordsZone[i][2] ) * ( coordsZone[i][3] - coordsZone[i][1] );
		else		area	 = ( coordsZone[i][0] - coordsZone[i][2] ) * ( coordsZone[i][3] - coordsZone[i][1] );

		if ( area <= area_old ) j = i;
		area_old = area;
	}

	bzero(url, 254);

	for ( k = 1; k < MAX_AIR_CRAFT_NUM; k++) {
		if ( ( AircraftDataArray[k].index != -1 ) && ( AircraftDataArray[k].status == USED ) ) {
			minTime =  AircraftDataArray[k].time < minTime ? AircraftDataArray[k].time : minTime;
		}
	}


	if ( minTime == INT_MAX ){
		if ( j != -1 ) 	sprintf(url, "http://db.flightradar24.com/zones/%s_all.js",   namesZone[j]); 
		else		sprintf(url, "http://db.flightradar24.com/zones/full_all.js");
	}else{
		if ( j != -1 ) 	sprintf(url, "http://db.flightradar24.com/zones/%s_all.js?callback=pd_callback&_=%d",   namesZone[j], minTime ); 
		else		sprintf(url, "http://db.flightradar24.com/zones/full_all.js?callback=pd_callback&_=%d",  minTime );
	}

	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 		0);
	curl_easy_setopt(curl_handle, CURLOPT_URL,		url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,    WriteMemoryCallback);     
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,        (void *)&chunk);

	printf("Downloading Flightradar24 information from %s ...\n", url);
	res = curl_easy_perform(curl_handle);
        if (res != CURLE_OK) {
                fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n", curl_easy_strerror(res));
		DownloadStatus = TRUE;
                return (void*)(1);
        }

	plane_code 	= (char *)malloc(sizeof(char ) *  CODE_LENGHT);

	cnt = 0;
	for ( token = strtok(chunk.memory, ","), i = 0, bzero(plane_code, CODE_LENGHT - 1); token != NULL; token = strtok(NULL, ",") ){
		if 	( i == 0  ) plane_code 		= strcpy(plane_code, token);
		else if	( i == 1  ) plane_lat 		= atof(token);
		else if ( i == 2  ) plane_lon 		= atof(token);
		else if ( i == 3  ) plane_course	= atof(token);
		else if ( i == 4  ) plane_ele		= atof(token) * 0.3048; 	// Convert feets to meters
		else if ( i == 5  ) plane_speed		= atof(token) * 0.514444444;	// knots to metes/sec;
		else if ( i == 10 ) plane_time		= atoi(token);

		if ( i >= 17 ){
			if ( plane_speed <= 0.0 ) { i = 0; bzero(plane_code, CODE_LENGHT - 1); continue; }
			if ( plane_ele   <= 0.0 ) { i = 0; bzero(plane_code, CODE_LENGHT - 1); continue; }

			if ( head == NULL ) 	{ head 		= (struct AircraftData *)malloc(sizeof(struct AircraftData)); cursor = head; 		}
			else			{ cursor->next 	= (struct AircraftData *)malloc(sizeof(struct AircraftData)); cursor = cursor->next; 	}


			cursor->name	= (char *)malloc(sizeof(char) * CODE_LENGHT); bzero(cursor->name, CODE_LENGHT - 1); 
			cursor->lat	= plane_lat;
			cursor->lon	= plane_lon;
			cursor->course	= plane_course;
			cursor->ele	= ( plane_ele < 0.0 ) ? 0.0 : plane_ele;
			cursor->speed	= plane_speed;
			cursor->vspeed	= 0.0;
			cursor->time	= plane_time;
			cursor->status	= FREE;
			cursor->next	= NULL;
			strcpy(cursor->name, plane_code);
			

			i = 0; bzero(plane_code, CODE_LENGHT - 1);
			cnt++;


		} else i++;
	}
	

	printf("Found %d airplanes ...\n", cnt);


 	while(e != head->next) {
 		c = a = head;
		b = a->next;
		while( a != e) {
			if( distAprox(myPlaneInfo.lon, myPlaneInfo.lat, a->lon, a->lat) > distAprox(myPlaneInfo.lon, myPlaneInfo.lat, b->lon, b->lat) ) {
				if(a == head) {
					tmp 	= b->next;
					b->next = a;
					a->next = tmp;
					head 	= b;
					c 	= b;
				} else {
					tmp 	= b->next;
					b->next = a;
					a->next = tmp;
					c->next = b;
					c 	= b;
				}
			} else {
				c = a;
				a = a->next;
			}
			b = a->next;
			if(b == e) e = a;
		}
		
 	}


	for ( cursor = head; cursor->next != NULL ; cursor = cursor->next ){
		dist = distAprox(myPlaneInfo.lon, myPlaneInfo.lat, cursor->lon, cursor->lat);
		if ( dist > MAX_AIR_CRAFT_DIST ) continue;

		for ( j = 1; j < MAX_AIR_CRAFT_NUM; j++) {
			if ( ! strcmp(AircraftDataArray[j].name, cursor->name) && ( AircraftDataArray[j].status == USED )) break;
		}


		if ( j == MAX_AIR_CRAFT_NUM ){
			for ( k = 1; k < MAX_AIR_CRAFT_NUM; k++) { if ( AircraftDataArray[k].status == FREE ) break; }
			if  ( k == MAX_AIR_CRAFT_NUM ) continue;
			printf("New ");
		
			AircraftDataArray[k].lat	= cursor->lat;
			AircraftDataArray[k].lon	= cursor->lon;
			AircraftDataArray[k].course	= cursor->course;
			AircraftDataArray[k].ele	= cursor->ele;
			AircraftDataArray[k].speed	= cursor->speed;
			AircraftDataArray[k].vspeed	= 0.0;
			AircraftDataArray[k].time	= cursor->time;
			AircraftDataArray[k].age	= NEW;
			AircraftDataArray[k].status	= USED;
			AircraftDataArray[k].obj 	= NULL;
			AircraftDataArray[k].itime	= 0.0;
			AircraftDataArray[k].index	= -1;
			strcpy(AircraftDataArray[k].name, cursor->name);
		}

		if ( j != MAX_AIR_CRAFT_NUM ){
			k = j;
			printf("Old ");
			AircraftDataArray[k].speed	= ( cursor->time > AircraftDataArray[k].time ) ? 
								distAprox(AircraftDataArray[k].lon, AircraftDataArray[k].lat, cursor->lon, cursor->lat) / (float)( cursor->time - AircraftDataArray[k].time ) * 1000.0 : 
								cursor->speed;
			

			AircraftDataArray[k].lat	= cursor->lat;
			AircraftDataArray[k].lon	= cursor->lon;
			AircraftDataArray[k].course	= cursor->course;
			AircraftDataArray[k].vspeed	= ( cursor->time > AircraftDataArray[k].time ) ? ( cursor->ele - AircraftDataArray[k].ele ) / (float)( cursor->time - AircraftDataArray[k].time ) : 0.0;
			AircraftDataArray[k].ele	= cursor->ele;
			AircraftDataArray[k].time	= cursor->time;
			AircraftDataArray[k].age	= ( cursor->time > AircraftDataArray[k].time ) ? NEW : OLD;
		
		} 

		AircraftDataArray[k].vspeed 	= ( AircraftDataArray[k].vspeed < 25.0 		) ? AircraftDataArray[k].vspeed : 0.0;
		AircraftDataArray[k].speed	= ( AircraftDataArray[k].speed  < cursor->speed ) ? AircraftDataArray[k].speed 	: cursor->speed;


printf("Aircraft %d: Coord: %f / %f,\tCourse: %d,\tSpeed: %f,\tvSpeed: %f,\tAlt: %f,\tDist: %f\n", AircraftDataArray[k].time, 	AircraftDataArray[k].lat, 	AircraftDataArray[k].lon, 	(int)AircraftDataArray[k].course,	
																AircraftDataArray[k].speed, 	AircraftDataArray[k].vspeed, 	AircraftDataArray[k].ele, dist );

	}

	for ( j = 1; j < MAX_AIR_CRAFT_NUM; j++) printf("AircraftDataArray[%d].status: %d, AircraftDataArray[%d] = %d\n", j, AircraftDataArray[j].status , j, AircraftDataArray[j].index ); 

	for ( j = 1; j < MAX_AIR_CRAFT_NUM; j++) {
		int toRemove = TRUE;
		for ( cursor = head; cursor->next != NULL ; cursor = cursor->next ){
			if ( ! strcmp(AircraftDataArray[j].name, cursor->name ) ) 								{ toRemove = FALSE; break; }
		}

		if ( distAprox( myPlaneInfo.lon, myPlaneInfo.lat, cursor->lon, cursor->lat) > MAX_AIR_CRAFT_DIST )				{ toRemove = TRUE;  break; }

		if ( toRemove == TRUE )	{
			bzero(AircraftDataArray[j].name, CODE_LENGHT - 1);
			AircraftDataArray[j].index 	= -1;
			AircraftDataArray[j].status 	= FREE;

		}
	}


	DownloadStatus = TRUE;
        return (void*)(1);
}


