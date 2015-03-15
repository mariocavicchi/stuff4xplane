#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <math.h>
#include <GL/gl.h>
#include <time.h>


#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMGraphics.h"
#include "XPLMDisplay.h"
#include "XPLMPlanes.h"

/*
00029f4c T tsrConnect
00029fd8 T tsrDisconnect
0002a7ac T tsrGetChannelInfoByID
0002a8bc T tsrGetChannelInfoByName
0002ab68 T tsrGetChannels
00029f10 T tsrGetLastError
0002a9d0 T tsrGetPlayerInfoByID
0002aa9c T tsrGetPlayerInfoByName
0002ac18 T tsrGetPlayers
0002a3d4 T tsrGetServerInfo
0002acc4 T tsrGetSpeakers
0002a6e4 T tsrGetUserInfo
0002a240 T tsrGetVersion
0002b2d4 T tsrKickPlayerFromChannel
0002b224 T tsrKickPlayerFromServer
0002a020 T tsrQuit
0002b438 T tsrSendTextMessage
0002b384 T tsrSendTextMessageToChannel
0002b040 T tsrSetOperator
0002ad94 T tsrSetPlayerFlags
0002b134 T tsrSetVoice
0002afa4 T tsrSetWantVoiceReason
0002a13c T tsrSwitchChannelID
0002a064 T tsrSwitchChannelName
*/



#define ONLINE			0
#define OFFLINE			1
#define NOTWORK			2
#define WAIT			3
#define GO			4
#define MAX_SERVERS_NUMBER 	10
#define	MAX_WHAZZUP_LINES	10000
#define MAX_SERVERS_LINES	50
#define TS_SAFE_TIME		3

#define MAX_ATC_DISTANCE	300000	// In meters
#define DELTA_FREQ		0.0001	
#define UPDATE_TIME		60	// Seconds

#define CONF_FILE		"./TSlink.conf"

char 	*pAircraft[21];
char 	*XIvApPath 	= NULL;
char 	*tsControlPath 	= NULL;
time_t 	tsSafeTime	= 0;	
int	tsStatus	= GO;

XPLMDataRef gPlaneX	= NULL;
XPLMDataRef gPlaneY	= NULL;
XPLMDataRef gPlaneZ	= NULL;
XPLMDataRef gPlaneLat 	= NULL;
XPLMDataRef gPlaneLon 	= NULL;
XPLMDataRef gPlaneEl	= NULL;
XPLMDataRef gCom1	= NULL;

char	*SERVERS[MAX_SERVERS_NUMBER];
char	*Whazzup[MAX_WHAZZUP_LINES];
char 	*TS_SERVERS[MAX_SERVERS_LINES];


struct MemoryStruct {
        char *memory;
        size_t size;
};

struct ATC{
	float	freq;
	char	name[25];
	float	lat;
	float	lon;
	char	server[100];
};


struct userInfo {
	char *VID;
	char 	*PASSWORD;
	char 	*CALLSIGN;
	float 	lat;
	float 	lon;
	float 	alt;
	float 	freq;
	float	time;
	float	clock;
	int 	status;
};



struct userInfo Pilot;


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

float distAprox(float lat1, float lon1, float lat2, float lon2) {
        float  R       = 6372.795477598;
        float  dLat    = 0.0;
        float  dLon    = 0.0;
        float  a       = 0.0;
        float  c       = 0.0;

        dLat = (lat2-lat1) * ( M_PI / 180.0 );
        dLon = (lon2-lon1) * ( M_PI / 180.0 );

        a       = sin(dLat/2.0) * sin(dLat/2.0) + cos(lat1 * ( M_PI / 180.0 )) * cos(lat2 * ( M_PI / 180.0 )) * sin(dLon/2.0) * sin(dLon/2.0);
        c       = 2.0 * atan2(sqrt(a), sqrt(1-a));

        return( R * c  * 1000.0);
}


int getFlightRadar24(){
	struct  	MemoryStruct chunk;
	CURL    	*curl_handle;
	CURLcode 	res;
	char		*token	= NULL;
	int		i 	= 0;
        float   	lat     = XPLMGetDataf(gPlaneLat);
        float   	lon     = XPLMGetDataf(gPlaneLon);
        //float   	el      = XPLMGetDataf(gPlaneEl);
	float		lat1	= 0;
	float		lon1	= 0;
	float		el1	= 0;


	chunk.memory    = NULL;
        chunk.size      = 0;
	curl_handle	= curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 		0);
	curl_easy_setopt(curl_handle, CURLOPT_URL,		"http://www.flightradar24.com/PlaneFeed.json");
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,    WriteMemoryCallback);     
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,        (void *)&chunk);

	res = curl_easy_perform(curl_handle);
        if (res != CURLE_OK) {
                fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n", curl_easy_strerror(res));
                return 1;
        }
/*
0 "EIN715":["4CA295"
1 51.5576
2 -1.3084
3 "278"
4 "16975"
5 "401"
6 "7663"
7 "EGLL2"
8 "A320"
9 "EI-DEO"
10 1329834243
11 "LHR"
12 "ORK"]
*/
	for ( token = strtok(chunk.memory, ","), i = 0; token != NULL; token = strtok(NULL, ",") ){

		if 	( i == 1 ) lat1 = atof(token);
		else if ( i == 2 ) lon1 = atof(token);
		else if ( i == 4 ) el1	= atof(token+1);

		if ( i >= 12 ){
			float dist = distAprox(lat, lon, lat1, lon1);
			if ( dist > MAX_ATC_DISTANCE ) { i = 0; continue; }

			printf("%f %f %f %f\n", lat1, lon1, el1, dist);

			i = 0;
		} else i++;

	}
	return 0;
}


int readConfigurationFile(){
	char 	line[1024];
	FILE 	*file = NULL;

	file = fopen (CONF_FILE, "r" );
	if ( file == NULL ){
		file = fopen(CONF_FILE, "w" );
		XIvApPath 	= (char *)malloc(sizeof(char) * 255);
		tsControlPath 	= (char *)malloc(sizeof(char) * 255);
		sprintf(XIvApPath,	"./Resources/plugins/X-IvAp Resources/X-IvAp.conf");
		sprintf(tsControlPath,	"./TeamSpeak2RC2/client_sdk/tsControl");


		fprintf(file, "XIvApPath\t= \"%s\"\n", 		XIvApPath);
		fprintf(file, "tsControlPath\t= \"%s\"\n", 	tsControlPath);
		fclose (file);
		return 0;
	}

	int start 	= 0;
	int end		= 0;
	int j		= 0;

	while ( fgets ( line, 1023, file ) != NULL ){
		for (  j = 0	; j < 1024; j++) if ( line[j] == '\"' ) { start	= j; break; }
		for (  j++	; j < 1024; j++) if ( line[j] == '\"' ) { end	= j; break; }

		if ( ( end - start ) <= 0) continue;
		
		if ( strstr(line, "XIvApPath") ){
			XIvApPath = (char *)malloc(sizeof(char) * (strlen(line) + 1) );
			sprintf(XIvApPath, "%.*s", end - start - 1, line + start + 1);
			continue;
	
		}

		if ( strstr(line, "tsControlPath") ){
			tsControlPath = (char *)malloc(sizeof(char) * (strlen(line) + 1) );
			sprintf(tsControlPath, "%.*s", end - start - 1, line + start + 1);
			continue;
	
		}
		
	}
	fclose ( file );
	
	if ( XIvApPath	   == NULL ) Pilot.status = NOTWORK;
	if ( tsControlPath == NULL ) Pilot.status = NOTWORK;


	return 0;
}


int findAlternateServer(char *idServ, char *name, char *output){
	int s = 0;
	int e = 0;

	// eu1.ts.ivao.aero/LEBB_EXA_APP
	for (int i = 0; i < MAX_SERVERS_LINES;  i++){
		if ( TS_SERVERS[i] == NULL ) 		continue;
		if ( ! strstr(TS_SERVERS[i], idServ) ) 	continue;
		for ( s = 0  	; s < (int)strlen(TS_SERVERS[i]); s++ ) if ( TS_SERVERS[i][s] == ':' ) break;
		for ( s++, e = s; e < (int)strlen(TS_SERVERS[i]); e++ ) if ( TS_SERVERS[i][e] == ':' ) break;
		if ( e <= s ) continue;
		sprintf(output, "%.*s/%s", e-s, TS_SERVERS[i]+s, name);

		//for (int j = 0; j < (int)strlen(idServ);  j++) idServ[j] = tolower(idServ[j]);
		//sprintf(output, "%s.ts.ivao.aero/%s", idServ, name);
		//printf("%s %s\n", output, TS_SERVERS[i]);

	}
	return 0;
}


int ExtractInfoFromLine(char *line, struct ATC *info){
	char 	*token	= NULL;
	char 	*tmp	= NULL;
	char	idServ[10];
	int	i	= 0;
	int	j	= 0;
	if ( line == NULL ) return 1;
	tmp = (char *)malloc(sizeof(char) * strlen(line) + 1);
	strcpy(tmp, line);
	info->freq 	= 0.0;
	info->lat	= 0.0;
	info->lon	= 0.0;
	bzero(info->name,   24);
	bzero(info->server, 99);

	for ( token = strtok(tmp, ":"), i = 0; token != NULL; token = strtok(NULL, ":"), i++ ){
		switch(i){
			case 0: // Name
				strcpy(info->name, token);
				break;
			case 4: // Freq

				info->freq = (float)((int)floor( atof(token) * 100 )) / 100.0; // Rounded to second decimal for X-Plane
				// Check VHF range
				if ( info->freq < 118.0	  ) return 1;
				if ( info->freq > 136.975 ) return 1;
			
				break;
			case 5: // Latitude
				info->lat = atof(token);
				// Check coordinate
				if ( info->lat < -90.0 ) return 1;
				if ( info->lat >  90.0 ) return 1;
				break;
			case 6: // Longitude
				info->lon = atof(token);
				// Check coordinate
				if ( info->lat < -180.0	) return 1;
				if ( info->lat >  180.0 ) return 1;
				break;
			case 9: // Server indetify 
				bzero(idServ, 9);
				strcpy(idServ, token);
				break;

			case 15: // Server
				for ( j = 0; j < (int)strlen(token); j++) if ( token[j] == '^' ) break;
				strncpy(info->server, token, j);
				// Check if the server is good
				if ( ! strstr( info->server, "ts.ivao.aero" )) {
 					findAlternateServer(idServ, info->name, info->server );
				}
				break;
			
		}

	}
	return 0;

}

int downloadServerList(){
	struct  	MemoryStruct chunk;
	CURL    	*curl_handle;
	CURLcode 	res;
	char		*token;
	int		i = 0;

	chunk.memory    = NULL;
        chunk.size      = 0;
	curl_handle	= curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 		0);
	curl_easy_setopt(curl_handle, CURLOPT_URL,		"http://www.ivao.aero/whazzup/status.txt");
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,    WriteMemoryCallback);     
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,        (void *)&chunk);

	res = curl_easy_perform(curl_handle);
        if (res != CURLE_OK) {
                fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n", curl_easy_strerror(res));
                return 1;
        }
	for ( token = strtok(chunk.memory, "\n"), i = 0; token != NULL; token = strtok(NULL, "\n") ){
		if ( ( token[0] == 'u' ) && ( strstr(token, "url0=" ) ) ){
			SERVERS[i] = (char *)malloc(sizeof(char) * strlen(token + 4));
			strcpy(SERVERS[i], token + 5);
			i++;
		}
	}
	return 0;
}

int downloadWhazzup(){
	struct  	MemoryStruct chunk;
	CURL    	*curl_handle;
	CURLcode 	res;
	char		*token;
	int		i = 0;
	int		j = 0;
	int		filePart = 0; // O: GENERAL, 1: CLIENTS, 2: AIRPORTS,  3: SERVERS 
	/*
	!GENERAL
	!CLIENTS
	!AIRPORTS
	!SERVERS
	*/

	chunk.memory    = NULL;
	chunk.size      = 0;
	curl_handle	= curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 		0);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,    WriteMemoryCallback);     
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,        (void *)&chunk);

	for ( i = 0, j = 0; i < MAX_SERVERS_NUMBER; i++){
		if ( SERVERS[i] == NULL ) continue;

		curl_easy_setopt(curl_handle, CURLOPT_URL, SERVERS[i]);

		res = curl_easy_perform(curl_handle);
	        if (res != CURLE_OK) { SERVERS[i] = NULL; continue; }

		for ( token = strtok(chunk.memory, "\n"); token != NULL; token = strtok(NULL, "\n") ){
			if ( strstr(token,"!GENERAL")  ) { filePart = 0; j = 0; continue; }
			if ( strstr(token,"!CLIENTS")  ) { filePart = 1; j = 0; continue; }
			if ( strstr(token,"!AIRPORTS") ) { filePart = 2; j = 0; continue; }
			if ( strstr(token,"!SERVERS")  ) { filePart = 3; j = 0; continue; }
						

			switch (filePart){
				case 1: // CLIENTS
					//if ( ! strstr(token, ":ATC:") ) continue;
					//if ( ! strstr(token, "ts.ivao.aero") ) continue;
					//printf("%s\n", token );
					Whazzup[j] = (char *)malloc(sizeof(char) * strlen(token) + 1);
					strcpy(Whazzup[j], token); j++;
					break;
				case 2: // AIRPORTS

					break;
				case 3: // SERVERS
					TS_SERVERS[j] = (char *)malloc(sizeof(char) * strlen(token) + 1);
					strcpy(TS_SERVERS[j], token); j++;
				default:
					break;
			}
		}
	}

	if ( j == 0 ) for (int i = 0; i < MAX_SERVERS_NUMBER; i++) SERVERS[MAX_SERVERS_NUMBER] = NULL;


	return 0;

}

int getInfoToST(){
	char commandLine[1024];
	char commandOut[1024];

	if ( tsStatus 	  != GO )	return 0;
	if ( Pilot.status != ONLINE ) 	return 0;


	tsStatus = WAIT;
        bzero(commandLine, 1023);
        bzero(commandOut,  1023);
        sprintf(commandLine, "%s GET_USER_INFO", tsControlPath);
	
	FILE 	*fp	= NULL;
	int 	error 	= 1;

	fp = popen(commandLine, "r");
	if (fp == NULL) { printf("Failed to run command: %s\n", commandLine ); return 1; }

	while (fgets(commandOut, 1023, fp) != NULL){
		for ( int j = 0; j < 128; j++){
			if ( commandOut[j] == '\n' ) commandOut[j] = '\0';
			if ( commandOut[j] == '\r' ) commandOut[j] = '\0';
		}

		if ( ! strcmp(commandOut, "OK" ) ) { error = 0; }
	}
	

	pclose(fp);

	if (error) Pilot.status = OFFLINE;
	
        return 0;


}





int disconnetToST(){
	char commandLine[1024];
	char commandOut[1024];

	if ( tsStatus 	  != GO )	return 0;
	if ( Pilot.status != ONLINE )	return 0;

        bzero(commandLine, 1023);
        bzero(commandOut,  1023);
        sprintf(commandLine, "%s DISCONNECT", tsControlPath);
        printf("Unlink TeamSpeak ...\n");
	tsStatus = WAIT;

	FILE 	*fp	= NULL;
	int 	error 	= 1;

	fp = popen(commandLine, "r");
	if (fp == NULL) { printf("Failed to run command: %s\n", commandLine ); return 1; }

	while (fgets(commandOut, 1023, fp) != NULL){
		for ( int j = 0; j < 128; j++){
			if ( commandOut[j] == '\n' ) commandOut[j] = '\0';
			if ( commandOut[j] == '\r' ) commandOut[j] = '\0';
		}

		if ( ! strcmp(commandOut, "OK" ) ) { error = 0; }
	}
	

	pclose(fp);

	if (error){ printf("Failed to run command: %s\n", commandLine ); Pilot.status = NOTWORK; return 1; }

	Pilot.VID 	= NULL; 
	Pilot.PASSWORD 	= NULL;
	Pilot.CALLSIGN	= NULL;
        Pilot.status	= OFFLINE;

        return 0;


}


int connectToST(char *name){
	char *tmp  	= NULL;
	char *host 	= NULL;
	char *channel 	= NULL;
	char commandLine[1024];
	char commandOut[1024];

	if ( tsStatus 	  != GO )	return 0;
	if ( Pilot.status != OFFLINE ) 	return 0;

	bzero(commandLine, 1023);
	bzero(commandOut,  1023);

	tmp = (char *)malloc(strlen(name) + 1);
	bzero(tmp, strlen(name));
	strcpy(tmp, name);
	host 	= strtok(tmp, "/");
	channel = strtok(NULL, "/");
	if ( host		== NULL ) return 1;
	if ( channel		== NULL	) return 1;
        if ( Pilot.VID 		== NULL ) return 1; 
	if ( Pilot.PASSWORD 	== NULL ) return 1;
	if ( Pilot.CALLSIGN	== NULL ) return 1;



	sprintf(commandLine, "%s CONNECT TeamSpeak://%s/?nickname=\"XP-%s\"?loginname=\"%s\"?password=\"%s\"?channel=%s", tsControlPath, host, Pilot.CALLSIGN, Pilot.VID, Pilot.PASSWORD, channel);
	printf("Link to %s on %s ...\n", channel, host);
	tsStatus = WAIT;

	FILE 	*fp	= NULL;
	int 	error 	= 1;

	fp = popen(commandLine, "r");
	if (fp == NULL) { printf("Failed to run command: %s\n", commandLine ); return 1; }

	while (fgets(commandOut, 1023, fp) != NULL){
		for ( int j = 0; j < 128; j++){
			if ( commandOut[j] == '\n' ) commandOut[j] = '\0';
			if ( commandOut[j] == '\r' ) commandOut[j] = '\0';
		}

		if ( ! strcmp(commandOut, "OK" ) ) { error = 0; }
	}
	

	pclose(fp);

	if (error){ printf("Failed to run command: %s\n", commandLine ); Pilot.status = NOTWORK; return 1; }
	

	Pilot.status = ONLINE;

	return 0;
}


int readXIvApInfo(){
	char 	line[128];
	FILE 	*file = NULL;

	if ( Pilot.status != OFFLINE ) return 0;

	file = fopen (XIvApPath , "r" );

	if ( file == NULL ){
		printf("Unable to open X-IvAp configuration file %s!\n", XIvApPath);
		Pilot.status = NOTWORK;
		return 1;
	}

	while ( fgets ( line, 127, file ) != NULL ){
		for ( int j = 0; j < 128; j++){
			if ( line[j] == '\n' ) line[j] = '\0';
			if ( line[j] == '\r' ) line[j] = '\0';
		}
		
		if ( strstr(line, "VID=") ){
			Pilot.VID = (char *)malloc(sizeof(char) * 255 );
			sprintf(Pilot.VID, "%s", line+4);
			continue;
	
		}

		if ( strstr(line, "PASSWORD=") ){
			Pilot.PASSWORD = (char *)malloc(sizeof(char) * 255 );
			sprintf(Pilot.PASSWORD, "%s", line+9);
			continue;
	
		}
		if ( strstr(line, "CALLSIGN=") ){
			Pilot.CALLSIGN = (char *)malloc(sizeof(char) * 255 );
			sprintf(Pilot.CALLSIGN, "%s", line+9);
			continue;
	
		}
		
	}
	fclose ( file );
	return 0;
}


float AcquireAircraftFlightLoopCB(float elapsedMe, float elapsedSim, int counter, void * refcon){
	int    PlaneCount;
	int Index;
	char FileName[256], AircraftPath[256];

	XPLMCountAircraft(&PlaneCount, 0, 0);
	if (PlaneCount <= 1) return 0;

	for (Index = 1; Index < PlaneCount; Index++){
		XPLMGetNthAircraftModel(Index, FileName, AircraftPath);   
		pAircraft[Index] = (char *)AircraftPath;

		if (XPLMAcquirePlanes((char **)&pAircraft, NULL, NULL)) XPLMSetAircraftModel(Index, AircraftPath);    
        }


	return 0;
}




float FlightLoopCallback( float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon){
	int	i 	= 0;
        float   elapsed = XPLMGetElapsedTime();
        float   lat	= XPLMGetDataf(gPlaneLat);
        float   lon	= XPLMGetDataf(gPlaneLon);
        float   el	= XPLMGetDataf(gPlaneEl);
	float	com1	= (float)XPLMGetDatai(gCom1) / 100.0;
	int	update	= 0;        
	time_t	now	= 0;



	Pilot.clock 	= elapsed; 	// Update "system" time 
	now		= time(NULL);	// Real system time

        //printf("Time=%f, lat=%f,lon=%f,el=%f com1=%.3f.\n",elapsed, lat, lon, el, com1);

	
        if ( Pilot.VID 		== NULL ) readXIvApInfo();
	if ( Pilot.PASSWORD 	== NULL ) readXIvApInfo();
	if ( Pilot.CALLSIGN	== NULL ) readXIvApInfo();



	
	double dist = distAprox(lat, lon, Pilot.lat, Pilot.lon);

	// Conditions for update
	if ( fabs( Pilot.clock - Pilot.time ) > UPDATE_TIME ) 	{ Pilot.time = Pilot.clock;	update = 1; 	}
	if ( dist > MAX_ATC_DISTANCE )				{ disconnetToST(); 		update = 1; 	}


	if ( ( ( now - tsSafeTime ) > TS_SAFE_TIME ) && ( tsStatus == WAIT ) ){ tsStatus = GO; 	tsSafeTime = now; }

	if (update){
		for (i = 0; i < MAX_SERVERS_NUMBER; i++) if ( SERVERS[i] != NULL ) break;
		if  (i == MAX_SERVERS_NUMBER ) downloadServerList();
		for (i = 0; i < MAX_WHAZZUP_LINES;  i++) if ( Whazzup[i] != NULL ) break;
		if  ( i == MAX_WHAZZUP_LINES )  downloadWhazzup();
		getInfoToST(); // Update and check connection

		//getFlightRadar24();
	}

	if ( ( fabs( com1 - Pilot.freq ) > DELTA_FREQ ) && ( Pilot.status == NOTWORK ) ) Pilot.status = OFFLINE;
	if (   fabs( com1 - Pilot.freq ) > DELTA_FREQ ) disconnetToST();




        Pilot.lat       = lat;
        Pilot.lon       = lon;
        Pilot.alt       = el;
        Pilot.freq      = com1;

	// Disconnect
	// 0.00 0.02 0.05 0.07 1.00

	

	for (i = 0; i < MAX_WHAZZUP_LINES;  i++){
		if ( Whazzup[i] == NULL ) continue;

		struct ATC info;
		if ( ExtractInfoFromLine(Whazzup[i], &info) ) { Whazzup[i] = NULL; continue; }

		double dist 	= distAprox(lat, lon, info.lat, info.lon);
		double distMax	= MAX_ATC_DISTANCE;

		if ( ( Pilot.status == OFFLINE ) && ( update ) ) printf("%8s\t %.2fMHz\t Server: %25s\t Dist: %8.1fKm\n", info.name, info.freq, info.server, dist / 1000.0);
		
		// For CTR or Radar control double distance
		if ( strstr(info.name, "CTR" ) ) distMax = MAX_ATC_DISTANCE * 2.0;
		if ( strstr(info.name, "OBS" ) ) distMax = MAX_ATC_DISTANCE * 2.0;

		if ( dist > distMax ) continue;

		if ( fabs( Pilot.freq - info.freq  ) < DELTA_FREQ ) connectToST(info.server);  
 	}

	if ( ( Pilot.status == OFFLINE ) && ( update ) ) printf("-------------------------------------------------------------------------------------\n");


        return 1.0;
}                            




int AircraftDrawCallback( XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon){
	/*
	XPLMDrawAircraft(
                                   int                  inPlaneIndex,    
                                   float                inX,    
                                   float                inY,    
                                   float                inZ,    
                                   float                inPitch,    
                                   float                inRoll,    
                                   float                inYaw,    
                                   int                  inFullDraw,    
                                   XPLMPlaneDrawState_t * inDrawStateInfo);    
	*/

	double  x,y,z;
	float 	Heading = 0, Pitch = 0, Roll = 0;
	XPLMPlaneDrawState_t inDrawStateInfo;

	int planeCount;
	XPLMCountAircraft(&planeCount, 0, 0);
	if ( planeCount <= 1 ) return 1;


	inDrawStateInfo.structSize	= sizeof(XPLMPlaneDrawState_t);
        inDrawStateInfo.gearPosition 	= 1;
        inDrawStateInfo.flapRatio 	= 1.0;
        inDrawStateInfo.spoilerRatio 	= 0;
        inDrawStateInfo.speedBrakeRatio = 0;
        inDrawStateInfo.slatRatio 	= 0;
        inDrawStateInfo.wingSweep 	= 0;
        inDrawStateInfo.thrust 		= 0;
        inDrawStateInfo.yokePitch 	= 0;
        inDrawStateInfo.yokeHeading 	= 0;
        inDrawStateInfo.yokeRoll 	= 0;

        x = XPLMGetDataf(gPlaneX);
        y = XPLMGetDataf(gPlaneY);
        z = XPLMGetDataf(gPlaneZ);
	glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(Heading, 	0.0, 	-1.0, 	0.0);
        glRotatef(Pitch, 	1.0, 	0.0, 	0.0);
        glRotatef(Roll, 	0.0, 	0.0, 	-1.0);

	XPLMDrawAircraft(1, x, y, z, 0, 0, 0, 1, &inDrawStateInfo);

	glPopMatrix();

	return 1;
}




PLUGIN_API int XPluginStart( char *outName, char *outSig, char *outDesc){

	strcpy(outName, "TeamSpeak2 to X-Plane Linker");
	strcpy(outSig,  "by Mario Cavicchi");
	strcpy(outDesc, "cavicchi@ferrara.linux.it");



	gPlaneLat = XPLMFindDataRef("sim/flightmodel/position/latitude");
        gPlaneLon = XPLMFindDataRef("sim/flightmodel/position/longitude");
        gPlaneEl  = XPLMFindDataRef("sim/flightmodel/position/elevation");
	gCom1	  = XPLMFindDataRef("sim/cockpit/radios/com1_freq_hz");
        gPlaneX	  = XPLMFindDataRef("sim/flightmodel/position/local_x");
        gPlaneY   = XPLMFindDataRef("sim/flightmodel/position/local_y");
        gPlaneZ   = XPLMFindDataRef("sim/flightmodel/position/local_z");

//	XPLMSetActiveAircraftCount(4); 
//	XPLMRegisterFlightLoopCallback(AcquireAircraftFlightLoopCB, 1.0, NULL);
//	XPLMRegisterDrawCallback( AircraftDrawCallback, xplm_Phase_Objects, 0, NULL);

	XPLMRegisterFlightLoopCallback(	FlightLoopCallback, 1.0, NULL);

        Pilot.VID	= NULL;
        Pilot.PASSWORD	= NULL;
        Pilot.CALLSIGN	= NULL;
        Pilot.lat	= -90.0;
        Pilot.lon	= -180.0;
        Pilot.alt	= 0.0;
        Pilot.freq	= 0.0;
	Pilot.status	= OFFLINE;
	Pilot.clock	= XPLMGetElapsedTime();
	Pilot.time	= Pilot.clock - UPDATE_TIME;
	tsStatus        = WAIT;
	tsSafeTime	= time(NULL);

	for (int i = 0; i < MAX_SERVERS_NUMBER; i++) SERVERS[i] 	= NULL;
	for (int i = 0; i < MAX_WHAZZUP_LINES;  i++) Whazzup[i]  	= NULL;
	for (int i = 0; i < MAX_SERVERS_LINES;  i++) TS_SERVERS[i]  	= NULL;

	readConfigurationFile();


	curl_global_init(CURL_GLOBAL_ALL);

	return (gCom1 != NULL) ? 1 : 0;
}




PLUGIN_API void XPluginDisable(void)	{}
PLUGIN_API int  XPluginEnable(void)	{ return 1; }
PLUGIN_API void XPluginStop(void)	{ disconnetToST(); XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL); }

