#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "connection.h"
#include "utils.h"

int begin_conn() {
	//socket
	if((fd_sock=socket(AF_UNIX, SOCK_STREAM, 0))==-1) {
		return -1;
	}
	//copying address
	struct sockaddr_un server_address;
	//setting memory
	memset(&server_address, '0', sizeof(server_address));
	server_address.sun_family=AF_UNIX; 
	strncpy(server_address.sun_path, SOCKETNAME, strlen(SOCKETNAME)+1);

	if((connect(fd_sock, (struct sockaddr*)&server_address, sizeof(server_address)))==-1) {
		return -1;
	}
	return 0;
}
int os_connect(char *name) {

	//CONNECTING
	int status=0;
	for(int i=0; i<MAXATTEMPT; i++) {
		if(begin_conn()==0) {
			status=0;
			break;
		}
		else status=-1;
	}
	if(status==-1) {
		perror("Error connecting to the server");
		return -1;
	}

	//request
	msg buf;
	buf.buffer=NULL;
	buf.length=strlen("REGISTER ")+strlen(name)+strlen(" \n");
	CHECK(buf.buffer, buf.length+1);
	sprintf(buf.buffer, "%s", "REGISTER ");
	strncat(buf.buffer, name, strlen(name));
	strncat(buf.buffer, " \n", strlen(" \n"));

	//writing
	int ret;
	if((ret=writen(fd_sock, buf.buffer, buf.length*sizeof(char)))==-1) {
		perror("Error writing request");
		free(buf.buffer);
		return -1;
	}
	free(buf.buffer);
	buf.buffer=NULL;

	//waiting answer
	CHECK(buf.buffer, MAXMSG_SIZE+1);
	if((ret=readn(fd_sock, buf.buffer, MAXMSG_SIZE*sizeof(char)))==-1) {
		perror("Error reading answer");
		free(buf.buffer);
		return -1;
	}

	//registration failed
	char *endptr;
	char *token=strtok_r(buf.buffer, " ", &endptr);
	if(memcmp(token, "KO", strlen("KO"))==0) {
		free(buf.buffer);
		return -1;
	}
	//registration success
	if(memcmp(token, "OK", strlen("OK"))==0) {
		free(buf.buffer);
		return 0;
	}
	free(buf.buffer);
	return 0;
}

int os_store(char *name, void *block, size_t len) {
	msg buf;
	char *tmp;
	CHECK(tmp, MAXMSG_SIZE+1);
	sprintf(tmp, "%ld", len);
	buf.length=strlen("STORE ")+strlen(name)+strlen(" ")+strlen(tmp)+strlen(" \n ")+(int)len;
	CHECK(buf.buffer, buf.length+1);
	sprintf(buf.buffer, "%s", "STORE ");
	strncat(buf.buffer, name, strlen(name));
	strncat(buf.buffer, " ", strlen(" "));
	strncat(buf.buffer, tmp, strlen(tmp));
	strncat(buf.buffer, " \n ", strlen(" \n "));
	free(tmp);

	int ret;
	//reading file
	char *bufread;
	CHECK(bufread, len+1);
	if((ret=readn((long)block, bufread, len*sizeof(char)))==-1) {
		perror("Error reading file");
		free(buf.buffer);
		free(bufread);
		return -1;
	}
	strncat(buf.buffer, bufread, strlen(bufread));
	memset(bufread, 0, len+1);
	free(bufread);

	//writing
	if((ret=writen(fd_sock, buf.buffer, buf.length*sizeof(char)))==-1) {
		perror("Error writing request");
		free(buf.buffer);
		return -1;
	}
	free(buf.buffer);
	buf.buffer=NULL;

	//waiting answer
	CHECK(buf.buffer, MAXMSG_SIZE+1);
	if((ret=readn(fd_sock, buf.buffer, MAXMSG_SIZE*sizeof(char)))==-1) {
		perror("Error reading answer");
		free(buf.buffer);
		return -1;
	}

	char *endptr;
	char *token=strtok_r(buf.buffer, " ", &endptr);
	if(strcmp(token, "KO")==0) {
		free(buf.buffer);
		return -1;
	}
	free(buf.buffer);
	return 0;
}
int os_delete(char *name) {
	//request
	msg buf;
	buf.length=strlen("DELETE ")+strlen(name)+strlen(" \n");
	CHECK(buf.buffer, buf.length+1);
	sprintf(buf.buffer, "%s", "DELETE ");
	strncat(buf.buffer, name, strlen(name));
	strncat(buf.buffer, " \n", strlen(" \n"));
	int ret;

	//writing
	if((ret=writen(fd_sock, buf.buffer, buf.length*sizeof(char)))==-1) {
		perror("Error writing request");
		free(buf.buffer);
		return -1;
	}
	free(buf.buffer);
	
	//waiting answer
	buf.length=MAXMSG_SIZE;
	CHECK(buf.buffer, MAXMSG_SIZE+1);
	if((ret=readn(fd_sock, buf.buffer, MAXMSG_SIZE*sizeof(char)))==-1) {
		perror("Error reading answer");
		free(buf.buffer);
		return -1;
	}
	//cancellation failed
	char *endptr;
	char *token=strtok_r(buf.buffer, " ", &endptr);
	if(strcmp(token, "KO")==0) {
		free(buf.buffer);
		return -1;
	}
	free(buf.buffer);
	return 0;
}

void *os_retrieve(char *name) {
	//request
	msg buf;
	buf.length=strlen("RETRIEVE ")+strlen(name)+strlen(" \n");
	CHECK(buf.buffer, buf.length+1);
	sprintf(buf.buffer, "%s", "RETRIEVE ");
	strncat(buf.buffer, name, strlen(name));
	strncat(buf.buffer, " \n", strlen(" \n"));

	//writing
	int ret;
	if((ret=writen(fd_sock, buf.buffer, buf.length*sizeof(char)))==-1) {
		perror("Error writing request");
		free(buf.buffer);
		return NULL;
	}
	free(buf.buffer);

	//waiting answer
	CHECK(buf.buffer, MAXMSG_SIZE+1);
	if((ret=readn(fd_sock, buf.buffer, MAXMSG_SIZE*sizeof(char)))==-1) {
		perror("Erro reading answer");
		free(buf.buffer);
		return NULL;
	}

	char *endptr;
	char *token=strtok_r(buf.buffer, " ", &endptr);
	if(strcmp(token, "KO")==0) {
		free(buf.buffer);
		return NULL;
	}
	char *datafile=NULL;
	if(strcmp(token, "DATA")==0) {
		token=strtok_r(NULL, " ", &endptr);
		int len=strtol(token, NULL, 10);
		token=strtok_r(NULL, " ", &endptr);
		token=strtok_r(NULL, "\0", &endptr);
		CHECK(datafile, len+1);
		int dimen=len-strlen(token);
		if(dimen>0) {
			char *string;
			CHECK(string, dimen+1);
			if((ret=readn(fd_sock, string, dimen*sizeof(char)))==-1) {
				perror("Error reading data");
				free(datafile);
				free(string);
				free(buf.buffer);
				return NULL;
			}
			sprintf(datafile, "%s", token);
			strncat(datafile, string, strlen(string));
			free(string);
			token=NULL;
		}
		else {
			sprintf(datafile, "%s", token);
		}
	}
	free(buf.buffer);
	return (void*)datafile;
}
int os_disconnect() {
	msg buf;
	buf.length=strlen("LEAVE \n");
	CHECK(buf.buffer, buf.length+1);
	sprintf(buf.buffer, "%s", "LEAVE \n");

	//writing
	if((writen(fd_sock, buf.buffer, buf.length*sizeof(char)))==-1) {
		perror("Error sending message");
		free(buf.buffer);
		return -1;
	}
	free(buf.buffer);

	//waiting answer
	CHECK(buf.buffer, MAXMSG_SIZE+1);
	if((readn(fd_sock, buf.buffer, buf.length*sizeof(char)))==-1) {
		free(buf.buffer);
		return 0;
	}

	//closing
	free(buf.buffer);
	close(fd_sock);
	return 0;
}
