#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

#define	BUFF_SIZE	1024
#define TS_SOCK		"/tmp/.teamspeakclient" 


int help(){
	printf("TeamSpeak 2 remote control by Mario Cavicchi <mariocavicchi@gmail.com>		\n");
	printf("Usage: tscontrol COMMAND [parameters] <optional>				\n");
	printf("Commands:									\n");
	printf("\n");
	printf("CONNECT [teamspeak_url]                             Connect to teamspeak server	\n");
	printf("   Example: CONNECT TeamSpeak://teamspeak.org?channelname=Channel%%201		\n");
	printf("DISCONNECT                                      Disconnect fromteamspeak server	\n");
	printf("QUIT                                                            Close teamspeak	\n");
	printf("SWITCH_CHANNEL [channelid] <password>                         Switch to channel	\n");
	printf("GET_CLIENT_VERSION                                       Returns Client Version	\n");
	printf("GET_SERVER_INFO                                      Returns info on the Server	\n");
	printf("GET_USER_INFO                         Returns info on the user of the TS client	\n");
	printf("GET_CHANNEL_INFO [channelid]               Returns info on the selected channel	\n");
	printf("GET_PLAYER_INFO [playerid]                  Returns info on the selected player	\n");
	printf("GET_CHANNELS                                 Returns a list of all the channels	\n");
	printf("GET_PLAYERS                                   Returns a list of all the players	\n");
	printf("GET_SPEAKERS                         Returns a list of all the current speakers	\n");
	printf("MUTE / UNMUTE                                    Mutes or unmutes your speakers	\n");
	printf("SET_OPERATOR [playerid] [GRANT/REVOKE] GRANT / REVOKE operator status to player	\n");
	printf("SET_VOICE [playerid] [GRANT/REVOKE]       GRANT / REVOKE Voice status to player	\n");
	printf("KICK_PLAYER_CHANNEL [playerid] <reason>          Kick a player from the channel	\n");
	printf("KICK_PLAYER_SERVER [playerid] <reason>            Kick a player from the server	\n");
	printf("SEND_MESSAGE_CHANNEL [channelid] [message]    Sends a text message to a channel	\n");
	printf("SEND_MESSAGE [message]                    Sends a text message to every channel	\n");
	return 0;
}


int main(int argc, char **argv){
	int i;
	struct 	sockaddr_un	name;
	int			sock;
	struct 	pollfd		ufds;
	char			*buff = NULL;
	int			size;

	if (argc < 2 ) { help(); return 1; }

	sock = socket (PF_FILE, SOCK_STREAM, 0);
	if (sock < 0) { perror ("ERROR: socket"); return 1; }


	name.sun_family = AF_LOCAL;
	strncpy (name.sun_path, TS_SOCK, sizeof(name.sun_path));

	if (connect(sock, (struct sockaddr *) &name, sizeof(name)) < 0) { perror("ERROR: connect"); return 1; }

	buff = (char *)malloc(sizeof(char) * BUFF_SIZE);
	if (buff == NULL) { perror ("ERROR: malloc"); return 1; }
	bzero(buff, BUFF_SIZE - 1);


	if 	  ( ! strcmp(argv[1], "GET_CLIENT_VERSION" ) ){

		size = 12;
		send(sock, &size, 			4,  0);
		send(sock, "GET_VERSION\0", 		12, 0);

	} else if ( ! strcmp(argv[1], "GET_SERVER_INFO" ) ){

		size = 16;
		send(sock, &size, 			4,  0);
		send(sock, "GET_SERVER_INFO\0", 	16, 0);

	} else if ( ! strcmp(argv[1], "DISCONNECT" ) ){

		size = 11;
		send(sock, &size, 			4,  0);
		send(sock, "DISCONNECT\0", 		11, 0);

	} else if ( ! strcmp(argv[1], "CONNECT"	) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "CONNECT %s", argv[2]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);

	} else if ( ! strcmp(argv[1], "QUIT" ) ){

		size = 5;
		send(sock, &size, 			4, 0);
		send(sock, "QUIT\0"	, 		5, 0);

	} else if ( ! strcmp(argv[1], "GET_CHANNELS" ) ){

		size = 13;
		send(sock, &size, 			4, 0);
		send(sock, "GET_CHANNELS\0", 		13,0);

	} else if ( ! strcmp(argv[1], "SWITCH_CHANNEL"	) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "SWITCH_CHANNEL_ID %s", argv[2]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);

	} else if ( ! strcmp(argv[1], "GET_USER_INFO" ) ){

		size = 14;
		send(sock, &size, 			4, 0);
		send(sock, "GET_USER_INFO\0", 		14,0);

	} else if ( ! strcmp(argv[1], "GET_CHANNEL_INFO" ) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "GET_CHANNEL_INFO_ID %s GET_PLAYERS", argv[2]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);

	} else if ( ! strcmp(argv[1], "GET_PLAYER_INFO" ) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "GET_PLAYER_INFO_ID %s", argv[2]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);

	} else if ( ! strcmp(argv[1], "GET_PLAYERS" ) ){

		size = 12;
		send(sock, &size, 			4, 0);
		send(sock, "GET_PLAYERS\0", 		12,0);

	} else if ( ! strcmp(argv[1], "MUTE" ) ){

		size = 44;
		send(sock, &size,				4, 0);
		send(sock, "SET_PLAYER_FLAGS pfInputMuted,pfO",	44,0);

	} else if ( ! strcmp(argv[1], "UNMUTE" ) ){

		size = 22;
		send(sock, &size,				4, 0);
		send(sock, "SET_PLAYER_FLAGS none\0",		22,0);

	} else if ( ! strcmp(argv[1], "GET_SPEAKERS" ) ){

		size = 13;
		send(sock, &size, 			4, 0);
		send(sock, "GET_SPEAKERS\0", 		13,0);

	} else if ( ! strcmp(argv[1], "SET_OPERATOR" ) ){
		if (argc < 4 ) { help(); return 1; }

		if 	( ! strcmp(argv[3], "GRANT"  ) ) sprintf(buff, "SET_OPERATOR %s grGrant",  argv[2]);
		else if	( ! strcmp(argv[3], "REVOKE" ) ) sprintf(buff, "SET_OPERATOR %s grRevoke", argv[2]);
		else	{ help(); return 1; }

		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);


	} else if ( ! strcmp(argv[1], "KICK_PLAYER_CHANNEL" ) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "KICK_PLAYER_FROM_CHANNEL %s", argv[2]);
		for (i = 3 ; i < argc ; i++) sprintf(buff, "%s %s", buff, argv[i]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);

	} else if ( ! strcmp(argv[1], "KICK_PLAYER_SERVER" ) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "KICK_PLAYER_FROM_SERVER %s", argv[2]);
		for (i = 3 ; i < argc ; i++) sprintf(buff, "%s %s", buff, argv[i]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);


	} else if ( ! strcmp(argv[1], "SEND_MESSAGE_CHANNEL" ) ){
		if (argc < 4 ) { help(); return 1; }

		sprintf(buff, "SEND_TEXT_MESSAGE_TO_CHANNEL %s", argv[2]);
		for (i = 3 ; i < argc ; i++) sprintf(buff, "%s %s", buff, argv[i]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);


	} else if ( ! strcmp(argv[1], "SEND_MESSAGE" ) ){
		if (argc < 3 ) { help(); return 1; }

		sprintf(buff, "SEND_TEXT_MESSAGE %s", argv[2]);
		for (i = 3 ; i < argc ; i++) sprintf(buff, "%s %s", buff, argv[i]);
		size = strlen(buff)+1;
		send(sock, &size, 			4,    0);
		send(sock, buff,			size, 0);

	} else { help(); return 1; }


	ufds.fd		= sock;
	ufds.events 	= POLLIN; 
	poll(&ufds, 1, 1000);

	recv(sock, &size, 	4, 	0);

	if (buff != NULL ) free(buff);
	buff = (char *)malloc(sizeof(char) * size);
	if (buff == NULL) { perror ("ERROR: malloc"); return 1; }
	bzero(buff, size - 1);


	recv(sock, buff, size, 	0);

	for( i= 0; i < strlen(buff) ; i++) if ( buff[i] == '\r' ) buff[i] = '\n';
	printf("%s\n", buff );

	return 0;
}
