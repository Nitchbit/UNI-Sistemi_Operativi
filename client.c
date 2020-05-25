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
	if(argc==1) {
		printf("Usage: %s registration_name #test\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if(strlen(argv[1])>MAXLEN_NAME) {
		perror("Error name length");
		return -1;
	}

	if((os_connect((char*)argv[1]))==-1) {
		perror("Error connection");
		return -1;
	}

	int opexec=0;
	int opsucc=0;
	int opfail=0;

	int ret;

	//test 1
	char pathtodir[strlen(argv[1])+strlen("/clients")];
	sprintf(pathtodir, "%s", "clients");
	strncat(pathtodir, "/", strlen("/"));
	strncat(pathtodir, (char*)argv[1], strlen((char*)argv[1]));
	if((strtol(argv[argc-1], NULL, 10))==1) {
		if((mkdir(pathtodir, S_IRWXU))==-1) {
			if((errno!=EEXIST)) {
				perror("Error creating directory");
				return -1;
			} 
		}
		char pathdir[UNIX_PATH_MAX+1];
		if(getcwd(pathdir, UNIX_PATH_MAX)==NULL) {
			perror("Error creating directory for client");
			return -1;
		}
		strncat(pathdir, "/clients", strlen("/clients"));
		strncat(pathdir, "/", strlen("/"));
		strncat(pathdir, (char*)argv[1], strlen(argv[1]));
		if((ret=chdir((const char*)pathdir))==-1) {
			perror("Error changing directory");
			return -1;
		}
		size_t dim=100;
		//creo gli oggetti e li spedisco
		for(int i=0; i<20; i++) {
			long fd;
			int name_dim=strlen("file")+sizeof(int);
			char name[name_dim+1];
			sprintf(name, "%s", "file");
			char tmp[sizeof(long)];
			sprintf(tmp, "%d", i+1);
			strncat(name, tmp, strlen(tmp));
			if((fd=open(name, O_CREAT | O_RDWR | O_APPEND | O_EXCL, 0777))==-1) {
				if(errno==17) {
					if((fd=open(name, O_RDWR, 0777))==-1) {
						perror("Error opening file");
						opexec++;
						opfail++;
						continue;
					}
				}
				else {
					perror("Error creating file");
					opexec++;
					opfail++;
					continue;
				}
			}
			size_t filedim=0;
			int i=0;
			while(filedim<dim) {
				if((ret=writen(fd, "Filler", strlen("Filler")*sizeof(char)))==-1) {
					perror("Error writing file");
					continue;
				}
				i++;
				if(i==10) {
					writen(fd, "\n", strlen("\n")*sizeof(char));
					filedim+=strlen("\n")*sizeof(char);
				}
				size_t strdim=strlen("Filler")*sizeof(char);
				filedim+=strdim;
			}
			lseek(fd, 0, SEEK_SET);
			if((os_store(name, (void*)fd, filedim))==-1) {
				perror("Error storing file");
				opfail++;
				opexec++;
			}
			else {
				opexec++;
				opsucc++;
			}
			close(fd);
			dim+=5300;
		}
	}
	//test 2
	if((strtol(argv[argc-1], NULL, 10))==2) {
		char pathdir[UNIX_PATH_MAX+1];
		if((getcwd(pathdir, UNIX_PATH_MAX))==NULL) {
			perror("Error getting current path");
			return -1;
		}
		strncat(pathdir, "/clients", strlen("/clients"));
		strncat(pathdir, "/", strlen("/"));
		strncat(pathdir, (char*)argv[1], strlen(argv[1]));
		if((chdir((const char*)pathdir))==-1) {
			perror("Error changing directory");
			return -1;
		}
		for(int i=0; i<20; i++) {
			int ret;
			int name_dim=strlen("file")+sizeof(int);
			char name[name_dim+1];
			sprintf(name, "%s", "file");
			char *tmp=(char*)malloc(sizeof(int));
			sprintf(tmp, "%d", i+1);
			strncat(name, tmp, strlen(tmp));
			free(tmp);
			char *puntdata;
			if((puntdata=os_retrieve(name))==NULL) {
				perror("Error retrieving file");
				opexec++;
				opfail++;
				continue;
			}
			long fd;
			if((fd=open(name, O_RDONLY))==-1) {
				perror("Error opening file");
				opexec++;
				opfail++;
				continue;
			}
			struct stat str;
			stat(name , &str);
			char memfile[str.st_size+1];
			memset(memfile, 0, str.st_size+1);
			if((ret=readn(fd, memfile, str.st_size*sizeof(char)))==-1) {
				perror("Error reading file");
				opexec++;
				opfail++;
				close(fd);
				continue;
			}
			close(fd);
			int res=0;
			if((res=memcmp(memfile, puntdata, str.st_size))==0) {
				opexec++;
				opsucc++;

			}
			else {
				opexec++;
				opfail++;
			}
			free(puntdata);
		}
	}
	//test 3
	if(strtol(argv[argc-1], NULL, 10)==3) {
		for(int i=0; i<20; i++) {
			int name_dim=strlen("file")+sizeof(int);
			char name[name_dim+1];
			sprintf(name, "%s", "file");
			char *tmp=(char*)malloc(sizeof(int));
			sprintf(tmp, "%d", i+1);
			strncat(name, tmp, strlen(tmp));
			free(tmp);
			if((os_delete(name))==-1) {
				perror("Error deleting file");
				opexec++;
				opfail++;
			}
			else {
				opexec++;
				opsucc++;
			}
		}
	}
	fprintf(stdout, "Nome client: %s\nTipo test: %s\n", argv[1], argv[2]);
	fprintf(stdout, "Operazioni eseguite: %d\nOperazioni completate: %d\nOperazioni fallite: %d\n", opexec, opsucc, opfail);
	int i=0;
	ret=0;
	while(i<MAXATTEMPT) {
		if((ret=os_disconnect())!=-1) break;
	}
	if(ret==-1) {
		perror("Error communicating disconnection");
	}
	return 0;
}

