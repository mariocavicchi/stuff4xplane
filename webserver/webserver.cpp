#include "core.h"

#define SERVER		"x-plane/1.0"
#define PROTOCOL	"HTTP/1.1"
#define RFC1123FMT	"%a, %d %b %Y %H:%M:%S GMT"
#define PORT 		8080

PLUGIN_API void	XPluginStop(void){}
PLUGIN_API void XPluginDisable(void){}
PLUGIN_API int XPluginEnable(void){ return 1; }
PLUGIN_API void XPluginReceiveMessage(	XPLMPluginID	inFromWho,
					long		inMessage,
					void 		*inParam){}


PLUGIN_API int XPluginStart( 	char *		outName,
				char *		outSig,
				char *		outDesc){

	pthread_t thread;


	strcpy(outName,	"GMaps Web Server");
	strcpy(outSig, 	"Mario Cavicchi");
	strcpy(outDesc, "http://members.ferrara.linux.it/cavicchi/GMaps/.");

	pthread_create(&thread, NULL, webServer, (void *)NULL);
 
	return 1;
}

