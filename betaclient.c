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

int main(int argc, char const *argv[]) {
	int dim=strlen((char*)argv[1]);
	char client[dim+1];
	sprintf(client, "%s", (char*)argv[1]);

	//printf("USAGE MESSAGE:\n");
	//printf("1) REGISTER name \\n\n2) STORE name len \\n data\n3) RETRIEVE name \\n\n4) DELETE name \\n\n5) LEAVE \\n\n");
	if(os_connect(client)==-1) goto exit;

	if(argc>2) goto test;
	//COMMUNICATION
	char buffer[MAXMSG_SIZE+1];
	int ret;
	while(1) {
		if((read(0, buffer, MAXMSG_SIZE*sizeof(char)))==-1) {
			perror("Error reading command");
			goto exit;
		}
		char *endptr;
		char *token=strtok_r(buffer, " ", &endptr);
		//REGISTER
		if(strcmp(token, "REGISTER")==0) {
			token=strtok_r(NULL, " ", &endptr);
			if((strcmp(token, client))!=0) {
				printf("Client linked to a different name");
			}
			FUN(ret, os_connect(token), "Error connecting");
			memset(buffer, 0, MAXMSG_SIZE+1);
		}
		//STORE
		if(strcmp(token, "STORE")==0) {
			char *name=strtok_r(NULL, " ", &endptr);
			struct stat buf;

			long fd;
			FUN(fd, open((const char*)name, O_CREAT|O_RDONLY, 0777), "Error opening file");
			stat(name, &buf);
			off_t len=buf.st_size;
			FUN(ret, os_store(name, (void*)fd, (size_t)len), "Error storing data");
			FUN(ret, close(fd), "Error closing file");
			memset(buffer, 0, MAXMSG_SIZE+1);
		}
		//DELETE
		if(strcmp(token, "DELETE")==0) {
			token=strtok_r(NULL, " ", &endptr);
			if((os_delete(token))==-1) goto exit;
			memset(buffer, 0, MAXMSG_SIZE+1);
		}
		//RETRIEVE
		char *string;
		if(strcmp(token, "RETRIEVE")==0) {
			token=strtok_r(NULL, " ", &endptr);
			if((string=(char*)os_retrieve(token))==NULL) {
				perror("Error retrieving file");
				goto exit;
			}
			long fd;
			FUN(fd, open(token, O_CREAT | O_WRONLY, 0777), "Error opening file");
			int status=-1;
			if((status=write(fd, string, strlen(string)*sizeof(char)))==-1) {
				perror("Error retrieving file");
				close(fd);
				goto exit;
			}
			if(status<0) {
				perror("Error retrieving file");
				close(fd);
				goto exit;
			}
			FUN(fd, close(fd), "Error closing file");
			memset(buffer, 0, MAXMSG_SIZE+1);
		}
		//LEAVE 
		if(strcmp(token, "LEAVE")==0) goto exitsucc;
		if(buffer) {
			memset(buffer, 0, MAXMSG_SIZE+1);
			free(buffer);
		}
	}
	return 0;
}