#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <stdlib.h>

#define UNIX_PATH_MAX 108
#define MAXMSG_SIZE 128
#define MAXLEN_NAME 32
#define SOCKETNAME "./objstore.sock"
#define MAXATTEMPT 5

#define CHECK(buffer, size) \
	buffer=(char*)malloc(size*sizeof(char)); \
	memset(buffer, 0, size);

typedef struct message{
	int length;
	char *buffer;
}msg;

//Variabile globale
long fd_sock;

int os_connect(char *name);
int os_store(char *name, void *block, size_t len); 
void *os_retrieve(char *name);
int os_delete(char *name);
int os_disconnect();

#endif