#ifndef _DOWNLOAD_H
#define _DOWNLOAD_H

// Standard include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// lib cURL include
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#define USER_AGENT              "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.2) Gecko/20100316 Firefox/3.6.2 GTB7.0"
#define TOKEN_STRING            "mSatelliteToken"
#define TIMEOUT_CONNECTION      30



struct MemoryStruct {
	char *memory;
	size_t size;
};



//static void	*myrealloc(void *ptr, size_t size);
int		initCurlHandle(CURL *curl_handle);
size_t		downloadItem(CURL *curl_handle, const char *url, unsigned char **itemData);

#endif
