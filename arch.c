#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>

#include "arch.h"
#include "utils.h"

client_queue *insert(char *name, int *status) {
	if(head==NULL) {
		client_queue *new=(client_queue*)malloc(sizeof(client_queue));
		new->client_id=(char*)malloc((MAXLEN_NAME+1)*sizeof(char));
		memset(new->client_id, 0, MAXLEN_NAME+1);
		sprintf(new->client_id, "%s", name);
		head=new;
		new->next=NULL;
		*status=0;
	}
	else {
		client_queue *corr=head;
		int trovato=0;
		while(corr!=NULL && trovato==0) {
			if((strcmp(corr->client_id, name))==0) {
				trovato=1;
				*status=-1;
				corr=NULL;
				return head;
			}
			else {
				corr=corr->next;
			}
		}
		if(trovato==0) {
			client_queue *new=(client_queue*)malloc(sizeof(client_queue));
			new->client_id=(char*)malloc((MAXLEN_NAME+1)*sizeof(char));
			memset(new->client_id, 0, MAXLEN_NAME+1);
			sprintf(new->client_id, "%s", name);
			new->next=head;
			head=new;
			*status=0;
		}
		corr=NULL;
	}
	return head;
}

client_queue *delet(char *name, int *status) {
	client_queue *corr=head, *prec=NULL;
	if(head==NULL) {
		*status=0;
		return head;
	}
	else {
		while(corr!=NULL) {
			if(strcmp(corr->client_id, name)==0) {
				if(prec==NULL) {
					head=corr->next;
					free(corr->client_id);
					free(corr);
					*status=0;
					return head;
				}
				else {
					prec->next=corr->next;
					free(corr->client_id);
					free(corr);
					*status=0;
					return head;
				}
			}
			else {	
				prec=corr;
				corr=corr->next;
			}
		}
	}
	*status=-1;
	return head;
}

struct dirent *search(char *path, char *name) {
	DIR *directory=NULL;
	if((directory=opendir(path))==NULL) {
		return NULL;
	}
	struct dirent *dir;
	if((dir=readdir(directory))==NULL) {
		if(errno!=0) return NULL;
	}
	while(dir!=NULL) {
		if(strcmp(dir->d_name, name)==0) break;
		else {
			if((dir=readdir(directory))==NULL) {
				if(errno!=0) return NULL;
			}
		}
	}
	closedir(directory);
	return dir;
}

//funzione di registrazione
int reg(long fd, char *name) {

	char pathdir[UNIX_PATH_MAX+1];
	if((getcwd(pathdir, UNIX_PATH_MAX))==NULL) {
		perror("Error getting current path");
		return -1;
	}
	struct dirent *clientdir;
	clientdir=search(pathdir, name);
	if(clientdir==NULL && errno==0) mkdir(name, S_IRWXU);

	int status;

	pthread_mutex_lock(&lock);
	head=insert(name, &status);
	pthread_mutex_unlock(&lock);

	if(status==0) {
		writen(fd, "OK \n", 4);
		pthread_mutex_lock(&lock_varglobal);
		client_conn++;
		pthread_mutex_unlock(&lock_varglobal);
	}
	return status;
}
//funzione di memorizzazione
int mem(long fd, char *name, char* size, char *data, char *client) {
	int ret;

	//ricerca directory client
	int lenpath=UNIX_PATH_MAX+strlen(client)+strlen(name)+(2*sizeof(char))+1;
	char pathdir[lenpath];
	if((getcwd(pathdir, UNIX_PATH_MAX))==NULL) {
		perror("Error obtaining path");
		return -1;
	}
	strncat(pathdir, "/", strlen("/"));
	strncat(pathdir, client, strlen(client));
	strncat(pathdir, "/", strlen("/"));
	strncat(pathdir, name, strlen(name));

	size_t dim=strtol(size, NULL, 10)-strlen(data);
	char stringtot[strtol(size, NULL, 10)+1];
	memset(stringtot, 0, strtol(size, NULL, 10)+1);
	if(dim>0) {
		char *datafile=(char*)malloc((dim+1)*sizeof(char));
		memset(datafile, 0, dim+1);
		if((ret=readn(fd, datafile, (int)dim*sizeof(char)))==-1) {
			free(datafile);
			perror("Error reading file");
			return -1;
		}
		sprintf(stringtot, "%s", data);
		strncat(stringtot, datafile, strlen(datafile));
		free(datafile);
	}
	else {
		sprintf(stringtot, "%s", data);
	}
	int fdfile;
	if((fdfile=open((const char*)pathdir, O_CREAT | O_EXCL | O_WRONLY, 0777))==-1) {
		if(errno!=EEXIST) {
			perror("Error opening file");
			return -1;
		}
		else {
			if((fdfile=open((const char*)pathdir, O_WRONLY))==-1) {
				perror("Error opening file");
				return -1;
			}
		}
	}
	int status=0;
	if((status=writen(fdfile, stringtot, strlen(stringtot)*sizeof(char)))==-1) {
		perror("Error writing file");
		return -1;
	}
	if((ret=close(fdfile))==-1) {
		perror("Error closing file");
		return -1;
	}
	if(status<1) {
		int len=strlen("KO memorization failed \n");
		writen(fd, "KO memorization failed \n", len*sizeof(char));
		return -1;
	}
	if(status>0) {
		writen(fd, "OK \n", 4);
	}

	pthread_mutex_lock(&lock_varglobal);
	numogg++;
	sizeobj_store+=strtol(size, NULL, 10);
	pthread_mutex_unlock(&lock_varglobal);

	return 0;
}
//funzione di cancellazione
int del(long fd, char *name, char *client) {
	//ricerca directory client
	int dimen=UNIX_PATH_MAX+strlen(client)+strlen(name)+(2*sizeof(char));
	char pathdir[dimen+1];
	if(getcwd(pathdir, UNIX_PATH_MAX)==NULL) {
		perror("Error creating directory for client");
		return -1;
	}
	strncat(pathdir, "/", strlen("/"));
	strncat(pathdir, client, strlen(client));
	int status=0;
	int ret;

	DIR *directory=NULL;
	if((directory=opendir(pathdir))==NULL) {
		return -1;
	}
	struct dirent *dir;
	if((dir=readdir(directory))==NULL) {
		if(errno!=0) return -1;
	}
	while(dir!=NULL) {
		if(strcmp(dir->d_name, name)==0) {
			status=1;
			break;
		}
		else {
			if((dir=readdir(directory))==NULL) {
				if(errno!=0) return -1;
			}
		}
	}
	strncat(pathdir, "/", strlen("/"));
	strncat(pathdir, name, strlen(name));

	struct stat str;
	stat(pathdir, &str);

	if(status==1) {
		if((ret=remove((const char*)pathdir))==-1) {
			perror("Error deleting file");
			closedir(directory);
			return -1;
		}
		writen(fd, "OK \n", 4);
	}
	if(status==0) {
		int len=strlen("KO no such a file or directory \n");
		writen(fd, "KO no such a file or directory \n", len*sizeof(char));
		return -1;
	}
	closedir(directory);

	pthread_mutex_lock(&lock_varglobal);
	numogg--;
	sizeobj_store-=str.st_size;
	pthread_mutex_unlock(&lock_varglobal);

	return 0;
}
int retr(long fd, char *name, char *client) {
	//ricerca directory client
	int dimen=UNIX_PATH_MAX+strlen(client)+strlen(name)+(2*sizeof(char));
	char pathdir[dimen+1];
	if(getcwd(pathdir, UNIX_PATH_MAX)==NULL) {
		perror("Error creating directory for client");
		return -1;
	}

	strncat(pathdir, "/", strlen("/"));
	strncat(pathdir, client, strlen(client));
	int ret;

	strncat(pathdir, "/", strlen("/"));
	strncat(pathdir, name, strlen(name));

	long fdfile;
	if((fdfile=open((const char*)pathdir, O_RDONLY))==-1) {	
		if(errno==2) {
			int len=strlen("KO no such a file or directory \n");
			writen(fd, "KO no such a file or directory \n", len*sizeof(char));
			return 0;
		}
		perror("Error opening file");
		return -1;
	}
	if(fdfile>0) {
		struct stat str;
		stat(pathdir, &str);
		char tmp[MAXMSG_SIZE+1];
		sprintf(tmp, "%ld", str.st_size);
		int len=strlen("DATA ")+strlen(tmp)+strlen(" \n ")+(int)str.st_size;
		char string[str.st_size+1];
		memset(string, 0, str.st_size+1);
		char buffer[len+1];
		memset(buffer, 0, len+1);
		sprintf(buffer, "%s", "DATA ");
		strncat(buffer, tmp, strlen(tmp));
		strncat(buffer, " \n ", strlen(" \n "));

		//reading file
		if((ret=readn(fdfile, string, str.st_size*sizeof(char)))==-1) {
			perror("Error reading file");
			return -1;
		}
		strncat(buffer, string, strlen(string));
		if((ret=close(fdfile))==-1) {
			perror("Error closing file");
			return -1;
		}
		if((ret=writen(fd, buffer, len*sizeof(char)))==-1) {
			perror("Error writing answer");
			return -1;
		}
	}
	return 0;
}
//funzione di disconnesione
int leave(char *name) {

	int status;
	pthread_mutex_lock(&lock);
	head=delet(name, &status);
	pthread_mutex_unlock(&lock);

	if(status==-1) return -1;
	else {	
		pthread_mutex_lock(&lock_varglobal);
		client_conn--;
		pthread_mutex_unlock(&lock_varglobal);
	}
	return 0;
}