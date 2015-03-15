#include "air_navigation_linux.h"

XPLMDataRef gGroundSpeed;
XPLMDataRef gAirSpeed;
XPLMDataRef gVertSpeed;
XPLMDataRef gPlaneHeading;
XPLMDataRef gPlaneAlt;  
XPLMDataRef gPlanePresAlt;
XPLMDataRef gPlaneLat;  
XPLMDataRef gPlaneLon;      
XPLMDataRef gYaw;		
XPLMDataRef gPitch;
XPLMDataRef gRoll;		
XPLMDataRef gSlip;
XPLMDataRef gAcc_x;
XPLMDataRef gAcc_y;
XPLMDataRef gAcc_z;



pthread_t 	pth[MAX_CLIENTS_NUM];
pthread_t 	avahiTh;
pthread_t	main_thread;
int		BridgeStatus;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

enum {
	startBridge = 1,
	stopBridge  = 2
};


int closeAll(){
	struct sockaddr_in 	pin;
	int			sd;

	if ( BridgeStatus != CLOSING ) return 1;


	printf("Air Navigator Pro Plugin: Cleaning avahi ...\n");
	if (group) 		avahi_entry_group_reset(group);
	if (group)		avahi_entry_group_free(group);
	if (client)		avahi_client_free(client);
	if (simple_poll) 	avahi_simple_poll_quit(simple_poll);
	if (simple_poll)	avahi_simple_poll_free(simple_poll);

	avahi_free(name); client = NULL; group = NULL;

	printf("Air Navigator Pro Plugin: Closing connections ...\n");
	BridgeStatus = STOP;
	memset(&pin, 0, sizeof(pin));
	pin.sin_family 		= AF_INET;
	pin.sin_addr.s_addr 	= inet_addr("127.0.0.1");
	pin.sin_port 		= htons(PORT);

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 		return 1;
	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1 ) 	return 1;

	send(sd, "stop", 4, 0); // Not important the message
	close(sd);

	// Release lock and set plugin stoped
	BridgeStatus = STOP;

	return 0;
}


//--------------------------------------------------------------------------------------------------------//

PLUGIN_API void XPluginReceiveMessage(	XPLMPluginID	inFromWho,
					long		inMessage,
					void 		*inParam){}


void MyMenuHandlerCallback(void *inMenuRef, void *inItemRef){
	int s = *((int*)(&inItemRef));
	switch(s){
		case startBridge:
			if ( BridgeStatus == STOP ){
				// Lock mutex to start plugin
				printf("Air Navigator Pro Plugin: Starting bridge ...\n");
				BridgeStatus = STARTING;
				pthread_create(&main_thread, NULL, webServer, (void *)NULL);

			} else printf("Air Navigator Pro Plugin: Plugin is already running ...\n");
			break;
		case stopBridge:
			if ( BridgeStatus == RUN ){
				// Lock mutex to shutdown
				printf("Air Navigator Pro Plugin: Stopping bridge ...\n");
				BridgeStatus = CLOSING;
				closeAll();

			} else printf("Air Navigator Pro Plugin: Plugin is already stopped ...\n");
			break;

	}

}


PLUGIN_API int XPluginStart( 	char *		outName,
				char *		outSig,
				char *		outDesc){

	int		mySubMenuItem;
	XPLMMenuID	myMenu;


	strcpy(outName,	"Air Navigation Pro");
	strcpy(outSig, 	"Mario Cavicchi");
	strcpy(outDesc, "http://members.ferrara.linux.it/cavicchi/");
	mySubMenuItem 	= XPLMAppendMenuItem( XPLMFindPluginsMenu(), "Air Navigation", 0, 1);
	myMenu 		= XPLMCreateMenu( "Air Navigation", XPLMFindPluginsMenu(), mySubMenuItem, MyMenuHandlerCallback, 0);


	XPLMAppendMenuItem( myMenu, "Start Air Navigation bridge", (void *) startBridge, 1);
	XPLMAppendMenuItem( myMenu, "Stop Air Navigation bridge",  (void *) stopBridge,	 1);


	gGroundSpeed    = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
	gAirSpeed	= XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
	gVertSpeed	= XPLMFindDataRef("sim/flightmodel/position/vh_ind");
	gPlaneHeading   = XPLMFindDataRef("sim/flightmodel/position/hpath");
	gPlaneAlt       = XPLMFindDataRef("sim/flightmodel/position/elevation");
	gPlanePresAlt	= XPLMFindDataRef("sim/flightmodel/misc/h_ind");
	gPlaneLat       = XPLMFindDataRef("sim/flightmodel/position/latitude");
	gPlaneLon       = XPLMFindDataRef("sim/flightmodel/position/longitude");
	gYaw		= XPLMFindDataRef("sim/flightmodel/position/phi");
	gPitch		= XPLMFindDataRef("sim/flightmodel/position/theta");
	gRoll		= XPLMFindDataRef("sim/flightmodel/position/phi");
	gSlip		= XPLMFindDataRef("sim/flightmodel/misc/slip");
	gAcc_x		= XPLMFindDataRef("sim/flightmodel/position/local_ax");
	gAcc_y		= XPLMFindDataRef("sim/flightmodel/position/local_ay");
	gAcc_z		= XPLMFindDataRef("sim/flightmodel/position/local_az");
	BridgeStatus	= STOP; 	// The plugin starts off

	return 1;
}

PLUGIN_API void XPluginStop(void){ 	closeAll(); BridgeStatus = STOP; }
PLUGIN_API void XPluginDisable(void){	closeAll(); BridgeStatus = STOP; }
PLUGIN_API int XPluginEnable(void){ return 1; }


//--------------------------------------------------------------------------------------------------------//

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
	assert(g == group || group == NULL);


	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
            		printf("Air Navigator Pro Plugin: Service '%s' successfully established.\n", name);
			break;

		case AVAHI_ENTRY_GROUP_COLLISION : {
				char *n = NULL;

            			n = avahi_alternative_service_name(name);
            			avahi_free(name);
            			name = n;
	
				printf("Air Navigator Pro Plugin: Service name collision, renaming service to '%s'\n", name);
		            	create_services(avahi_entry_group_get_client(g));
	            		break;
        		}

		case AVAHI_ENTRY_GROUP_FAILURE :

            		printf("Air Navigator Pro Plugin: Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
            		avahi_simple_poll_quit(simple_poll);
	            	break;

		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
            		;
    	}
}



static void create_services(AvahiClient *c) {
	int ret;
	assert(c);

	if (!group) if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
		printf("Air Navigator Pro Plugin: avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
		goto fail;
	}

	printf("Air Navigator Pro Plugin: Adding service '%s'\n", name);

	if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, name, AVAHI_SERVICE_TYPE, NULL, NULL, PORT, NULL)) < 0) {
		printf("Air Navigator Pro Plugin: Failed to add _printer._tcp service: %s\n", avahi_strerror(ret));
		goto fail;
	}

	if ((ret = avahi_entry_group_commit(group)) < 0) {
		printf("Air Navigator Pro Plugin: Failed to commit entry_group: %s\n", avahi_strerror(ret));
		goto fail;
	}

	return;

	fail:
	avahi_simple_poll_quit(simple_poll);
}





static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
	assert(c);

	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:
			if (!group) create_services(c);
			break;

		case AVAHI_CLIENT_FAILURE:
			printf("Air Navigator Pro Plugin: Client failure %s\n", avahi_strerror(avahi_client_errno(c)));
			avahi_simple_poll_quit(simple_poll);
			break;

		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			if (group) avahi_entry_group_reset(group);
			break;
		case AVAHI_CLIENT_CONNECTING: 
			;
	}
}


static void modify_callback(AVAHI_GCC_UNUSED AvahiTimeout *e, void *userdata) {
	AvahiClient *client = (AvahiClient*)userdata;

	printf("Air Navigator Pro Plugin: Doing some weird modification\n");
	avahi_free(name);
	name = avahi_strdup(AVAHI_SERVICE_NAME);

	if (avahi_client_get_state(client) == AVAHI_CLIENT_S_RUNNING) {

        	if (group) avahi_entry_group_reset(group);
		create_services(client);
	}
}



// void *avahiService(void *arg){	avahi_simple_poll_loop(simple_poll); return (void*)(1); }
void *avahiService(void *arg){
	while (1){
		if ( BridgeStatus <= CLOSING ) { printf("Air Navigator Pro Plugin: Received STOP during avahi loop ...\n");  break; }
		if ( avahi_simple_poll_iterate(simple_poll, 1000*10 ) != 0 ) break;
	}
	return (void*)(1);
}
//--------------------------------------------------------------------------------------------------------//

void *webServer(void *arg){
	struct sockaddr_in 	sin;
	int 			s;
	int 			i = 1;
	int 			sock;
	int			error;
	int			ret = 1;
	struct timeval tv;

	if ( BridgeStatus != STARTING ) return (void*)(1);


	if (!(simple_poll = avahi_simple_poll_new())) { printf("Air Navigator Pro Plugin: Failed to create simple poll object.\n"); goto fail; }

	name 	= avahi_strdup(AVAHI_SERVICE_NAME);
	client 	= avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0, client_callback, NULL, &error);

	if (!client) { printf("Air Navigator Pro Plugin: Failed to create client: %s\n", avahi_strerror(error)); goto fail; }


	avahi_simple_poll_get(simple_poll)->timeout_new(	avahi_simple_poll_get(simple_poll),
								avahi_elapse_time(&tv, 1000*10, 0),
								modify_callback,
								client);


	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family 		= AF_INET;
	sin.sin_addr.s_addr 	= INADDR_ANY;
	sin.sin_port 		= htons(PORT);

	if ( bind(sock, (struct sockaddr *) &sin, sizeof(sin)) ) return (void*)(1);
	if ( listen(sock, 5) ) 					 return (void*)(1);
	for ( i = 0; i < MAX_CLIENTS_NUM; i++ ) pth[i] = 0;

	pthread_create(&avahiTh, NULL, avahiService, NULL);

	printf("Air Navigator Pro Plugin: listening on port %d\n", PORT );
	BridgeStatus = RUN;
	
	i = 0;	while (1){
		s = accept(sock, NULL, NULL); 
		if ( BridgeStatus <= CLOSING ) { printf("Air Navigator Pro Plugin: Received STOP in listing section ...\n"); break; }
		if (s < 0) continue;
		if ( i >= MAX_CLIENTS_NUM ) { close(s); continue; } 
		pthread_create(&(pth[i]), NULL, process, (void *)&s);
		i++;
	}
	
	close(s);
	close(sock);
	return (void*)(1);


	fail:
		if (client)		avahi_client_free(client);
		if (simple_poll)	avahi_simple_poll_free(simple_poll);
		avahi_free(name);
		BridgeStatus = STOP;

	return (void*)(1);
}

void *process(void *sock){
	char 		*buf[50];
	char 		*command	= NULL;
	int		i		= 0;
	int		error		= 0;
	char		data[4096];
	struct sockaddr_in addr;

	double		Speed		= 0.0;
	double		AirSpeed	= 0.0;
	double		VertSpeed	= 0.0;
	double		Heading		= 0.0;
	double		Altitude	= 0.0;
	double		PressAltitude	= 0.0;
	double		Latitude	= 0.0;
	double		Longitude	= 0.0;
	double		Yaw		= 0.0;
	double		Pitch		= 0.0;
	double		Roll		= 0.0;
	double		Slip		= 0.0;
	double		Acc_x		= 0.0;
	double		Acc_y		= 0.0;
	double		Acc_z		= 0.0;


	int		s		= *(int *)sock;


	socklen_t socklen = sizeof(addr); 
	socklen_t len	  = sizeof(error);
	if ( getpeername(s, (struct sockaddr*) &addr, &socklen) < 0) 	printf("Air Navigator Pro Plugin: Accept new connection from unknown ...\n");
	else 								printf("Air Navigator Pro Plugin: Accept new connection from %s:%d ...\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));


	while (1){
		if ( i >= 50 ) break;
		if ( ( buf[i] = (char *)malloc(sizeof(char) * 4096) ) == NULL )	{ close(s); return  (void*)(1); }
		bzero(buf[i], 4095);
		ssize_t bytes_recieved = recv(s, buf[i], 4096, 0);
		buf[i][bytes_recieved] = '\0';
		if ( bytes_recieved <= 0  ) return (void*)(1);
		if (( buf[i][0] == '\r' ) && ( buf[i][1] == '\n' )) break; 
		i++;
	}

        command = strtok(buf[0], "\n");
       	if (!command) { close(s); return (void*)(1); }
	if ( ! strstr( buf[0], "{\"cmd\"=\"getdata\"}" ) ) { close(s); return (void*)(1); }


	while(1){
		if ( BridgeStatus <= CLOSING ) { printf("Air Navigator Pro Plugin: Received STOP during connection ...\n");  break; }
		if ( getsockopt (s, SOL_SOCKET, SO_ERROR, &error, &len ) != 0 ) break;

	        Latitude	= XPLMGetDataf(gPlaneLat); if ( isnan(Latitude) != 0 ) break;
	        Speed           = XPLMGetDataf(gGroundSpeed);
	        AirSpeed        = XPLMGetDataf(gAirSpeed);
		VertSpeed	= XPLMGetDataf(gVertSpeed);
	        Altitude	= XPLMGetDataf(gPlaneAlt);
		PressAltitude	= XPLMGetDataf(gPlanePresAlt) * 0.3048; // Conversion from feets to meters
	        Heading		= XPLMGetDataf(gPlaneHeading);
	        Longitude	= XPLMGetDataf(gPlaneLon);
		Yaw		= XPLMGetDataf(gYaw);
		Pitch		= XPLMGetDataf(gPitch);
		Roll		= XPLMGetDataf(gRoll);
		Slip		= XPLMGetDataf(gSlip);
		Acc_x		= XPLMGetDataf(gAcc_x) / 9.80665; // Conversion to mtr/sec2 to G
		Acc_y		= XPLMGetDataf(gAcc_y) / 9.80665;
		Acc_z		= XPLMGetDataf(gAcc_z) / 9.80665;

		bzero(data, 4095);
		sprintf(data,"{ " 								); 
		sprintf(data,"%s \"%s\": %ld, ", data, JSON_GPS_TIMESTAMP,	time(NULL) 	); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_GROUNDSPEED,	Speed		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_AIRSPEED,		AirSpeed	);
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_COURSE, 		Heading 	); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_ALTITUDE, 		Altitude 	); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_PRESSURE_ALTITUDE,	PressAltitude 	); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_VERTICAL_ACC, 	1.0 		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_HORIZONTAL_ACC, 	1.0 		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_LATITUDE,		Latitude 	); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_LONGITUDE, 		Longitude	); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_YAW, 		Yaw		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_PITCH, 		Pitch		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_ROLL, 		Roll		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_SLIP, 		Slip		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_ACC_X, 		Acc_x		); 
		sprintf(data,"%s \"%s\": %f, ",  data, JSON_ACC_Y, 		Acc_y		); 
		sprintf(data,"%s \"%s\": %f " ,  data, JSON_ACC_Z, 		Acc_z		); 
		sprintf(data,"%s }\r\n\r\n", 	 data 		      				); 

		if ( send(s,  data,strlen(data), 0) <= 0 ) break;

		usleep((int)(1000000/FREQ_HZ));
	}

	if ( getpeername(s, (struct sockaddr*) &addr, &socklen) < 0) 	printf("Air Navigator Pro Plugin: Close connection with unknown ...\n");
	else 								printf("Air Navigator Pro Plugin: Close connection with %s:%d ...\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	close(s);
	return (void*)(1);
}


