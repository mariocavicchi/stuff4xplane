#include "download.h"


static void *myrealloc(void *ptr, size_t size){
	if(ptr)	return realloc(ptr, size);
	else	return malloc(size);
}
 
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data){
	size_t realsize			= size * nmemb;
	struct MemoryStruct *mem	= (struct MemoryStruct *)data;
 
	mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

int initCurlHandle(CURL    *curl_handle){
	int	i, j;
	int	offset			= 0;
	char 	key[]			= TOKEN_STRING;
	char	*token			= NULL;
	char	*SatelliteToken 	= NULL;
	char	nline[256]		= {};
	struct	MemoryStruct chunk;
	struct	curl_slist *cookie	= NULL;
	struct	curl_slist *cursor	= NULL;
	struct	curl_slist *headers	= NULL; 
	CURLcode res;

	chunk.memory	= NULL; 
	chunk.size	= 0;    


	headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	headers = curl_slist_append(headers, "Accept-Language: en-us,en;q=0.5");
	headers = curl_slist_append(headers, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7");
	headers = curl_slist_append(headers, "Keep-Alive: 115");
	headers = curl_slist_append(headers, "Connection: keep-alive");

	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 		0);				// Verbose NO
	curl_easy_setopt(curl_handle, CURLOPT_URL,		"http://maps.google.com");	// URL
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,	WriteMemoryCallback);		// Headler to function to write in memory
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,	(void *)&chunk);		// Pointer to memory where write
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT,	USER_AGENT);			// Modifed User agent
	curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST,	"ALL");  			// Clean cookie list
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, 	"");				// Set where store cookie 
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER,	headers);			// Modifed header like firefox 3.6
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT,		TIMEOUT_CONNECTION); 		// set timeout connection
	res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK) {
		fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n", curl_easy_strerror(res));
		return 1;
	}

  
	//------------------------------------------------------------------


	// Search satellite key	
	for (i = 0 ; i < (int)chunk.size; i++){
		if ( chunk.memory[i] != key[0] ) continue;
		offset = i;
		for (j = 1, i++ ; j < (int)strlen(key) ; j++, i++){
			if ( chunk.memory[i] != key[j] ) break;
		}
		if ( j == (int)strlen(key) ) break;
	}

	// if not found iterator is equal to size
	if ( i == (int)chunk.size ) return 1;

	for ( token = strtok(chunk.memory+offset, "\""), i = 0; token != NULL; token = strtok(NULL, "\""), i++ ){
		if ( i == 1 ){ // First after the variable name
			SatelliteToken = (char *)malloc(sizeof(char) * (strlen(token)+1));
			strcpy(SatelliteToken, token);
			break;
		}
	}

	if(chunk.memory)  free(chunk.memory);
	
	//------------------------------------------------------------------
	curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "ALL");  // Clean cookie list
	cursor = cookie, i = 1;  
	while (cursor) {  
		curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, cursor->data);
		cursor = cursor->next;  
		i++;  
	}  

	snprintf(nline, 256, "%s\t%s\t%s\t%s\t%d\t%s\t%s",  ".google.com", "TRUE", "/vt/lbw",		"FALSE", 0, "khcookie", SatelliteToken);  
	res = curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, nline);  
	if (res != CURLE_OK) {  fprintf(stderr, "Curl curl_easy_setopt failed: %s\n", curl_easy_strerror(res)); return 1; }  

	snprintf(nline, 256, "%s\t%s\t%s\t%s\t%d\t%s\t%s",  ".google.com", "TRUE", "/maptilecompress",	"FALSE", 0, "khcookie", SatelliteToken);  
	res = curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, nline);  
	if (res != CURLE_OK) {  fprintf(stderr, "Curl curl_easy_setopt failed: %s\n", curl_easy_strerror(res)); return 1; }  

	snprintf(nline, 256, "%s\t%s\t%s\t%s\t%d\t%s\t%s",  ".google.com", "TRUE", "/kh",		"FALSE", 0, "khcookie", SatelliteToken);  
	res = curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, nline);  
	if (res != CURLE_OK) {  fprintf(stderr, "Curl curl_easy_setopt failed: %s\n", curl_easy_strerror(res)); return 1; }  


	curl_slist_free_all(cookie);  
	curl_slist_free_all(cursor);  
	return 0;
}

size_t downloadItem(CURL *curl_handle, const char *url, unsigned char **itemData){

	CURLcode	res;
	struct		MemoryStruct buffer;
	size_t		size = 0;

	buffer.memory	= NULL; 
	buffer.size	= 0;    


	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,	WriteMemoryCallback);	// Headler to function to write in memory
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,	(void *)&buffer);	// Pointer to memory where write
	curl_easy_setopt(curl_handle, CURLOPT_URL,		url);			// Set url to download
	res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK) {
		fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n", curl_easy_strerror(res));
		return 0;
	}
	size 	 	= buffer.size;
	(*itemData)	= (unsigned char *)malloc(size);

	memcpy((*itemData), buffer.memory, size);
	if(buffer.memory)  free(buffer.memory);

	return size;

}

