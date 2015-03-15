#ifndef _CORE_H
#define _CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"



#define SERVER          "x-plane/1.0"
#define PROTOCOL        "HTTP/1.1"
#define RFC1123FMT      "%a, %d %b %Y %H:%M:%S GMT"
#define PORT            8080

void    *webServer(void *);
int     process(FILE *);
void    send_headers(   FILE *, int , const char *, const char *, const char *, int );
void    send_error(     FILE *, int , const char *, const char *, const char *);

#endif
