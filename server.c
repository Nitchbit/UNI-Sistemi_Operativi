#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

static int sig=0;
static sigset_t set;
sigset_t setsig;

//thread listener, sta in ascolto di nuove connessioni
void *listener(void *arg);
//thread worker, gestisce tutte le richieste di un client per l'intera durata della connessione
void *worker(void *arg);

void cleanup() {
    unlink(SOCKETNAME);
}

//funzione di gestione dei segnali
static void sighandler(int signal) {
	if(signal==SIGSEGV) {
		perror("SIGSEGV received");
		server_online=0;
	}
	if(signal==SIGINT) {
		perror("SIGINT received");
		server_online=0;
	}
	if(signal==SIGTERM) {
		perror("SIGTERM received");
		server_online=0;
	} 
	if(signal==SIGUSR1) {
		pthread_mutex_lock(&lock_varglobal);
		fprintf(stdout, "SIGUSR1 received:\nClient connessi: %d\nNumero oggetti: %d\nDimensione store: %ld Byte\n", client_conn, numogg, (size_t)sizeobj_store);			pthread_mutex_unlock(&lock_varglobal);
		pthread_mutex_unlock(&lock_varglobal);

		sigemptyset(&set);			
		sigaddset(&set, SIGINT);
		sigaddset(&set, SIGTERM);
		sigaddset(&set, SIGUSR1);
		sigwait(&set, &sig);
		sighandler(sig);
	}
}

int main(int argc, char const *argv[]) {
	cleanup();    
    atexit(cleanup);

    if((sigfillset(&setsig))==-1) {
    	perror("Error setting signal");
    	exit(EXIT_FAILURE);
    }
    //blocco i segnali in preparazione del server
    if((pthread_sigmask(SIG_BLOCK, &setsig, NULL))!=0) {
		perror("Error sigmask");
		_exit(-1);
	}

    server_online=1;

    head=NULL;
    if((pthread_mutex_init(&lock, NULL))!=0) {
    	perror("Error initializing lock");
    	exit(EXIT_FAILURE);
    }
    if((pthread_mutex_init(&lock_varglobal, NULL))!=0) {
    	perror("Error initializing lock");
    	exit(EXIT_FAILURE);
    }

	int ret;
	//STARTING SERVER
	if((mkdir("data", S_IRWXU))==-1) {
		if(errno!=EEXIST) {
			perror("Error creating file system");
			pthread_mutex_destroy(&lock);
			pthread_mutex_destroy(&lock_varglobal);
			exit(EXIT_FAILURE);
		}
	}
	if((mkdir("clients", S_IRWXU))==-1) {
		if(errno!=EEXIST) {
			perror("Error creating file system");
			pthread_mutex_destroy(&lock);
			pthread_mutex_destroy(&lock_varglobal);
			exit(EXIT_FAILURE);
		}
	}

	if((ret=pthread_mutex_lock(&lock_varglobal))!=0) {
		perror("Error locking lock_varglobal");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		exit(EXIT_FAILURE);
	}
	client_conn=0;
	numogg=0;
	sizeobj_store=0;
	if((ret=pthread_mutex_unlock(&lock_varglobal))!=0) {
		perror("Error unlocking lock_varglobal");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		exit(EXIT_FAILURE);
	}
	
	//socket
	errno=0;
	long fd_sock;
	if((fd_sock=socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0))==-1) {
		perror("Error creating socket");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		exit(EXIT_FAILURE);
	}
	//copying address
 	struct sockaddr_un server_address;
	memset(&server_address, '0', sizeof(server_address));
	server_address.sun_family=AF_UNIX; 
	strncpy(server_address.sun_path, SOCKETNAME, strlen(SOCKETNAME)+1);

	//bind
	if((ret=bind(fd_sock, (struct sockaddr*)&server_address, sizeof(server_address)))==-1) {
		perror("Error binding socket");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		close(fd_sock);
		exit(EXIT_FAILURE);
	}
	//listen
	if((ret=listen(fd_sock, SOMAXCONN))==-1) {
		perror("Error listening socket");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		close(fd_sock);
		exit(EXIT_FAILURE);
	}

	pthread_t listentid;
	if((ret=pthread_create(&listentid, NULL, &listener, (void*)fd_sock))!=0) {
		perror("Error creating thread");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		close(fd_sock);
		exit(EXIT_FAILURE);
	}

	//blocco il segnale di broken pipe e mi metto in ascolto di SIGINT, SIGTERM, SIGSEGV, SIGUSR1
	signal(SIGPIPE, SIG_IGN);

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGSEGV);
	if((sigwait(&set, &sig))!=0) {
		perror("Error waiting for signal");
		_exit(-1);
	}
	sighandler(sig);
	if((ret=pthread_join(listentid, NULL))!=0) {
		perror("Error joining thread");
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&lock_varglobal);
		close(fd_sock);
		exit(EXIT_FAILURE);
	}
	while(client_conn>0) {
		sleep(1);
	}

	//pulisco il server prima di chiudere
	client_queue *corr=head;
	pthread_mutex_lock(&lock);
	while(corr!=NULL) {
		free(corr->client_id);
		head=corr->next;
		free(corr);
		corr=head;
	}
	pthread_mutex_unlock(&lock);

	pthread_mutex_destroy(&lock);
	pthread_mutex_destroy(&lock_varglobal);
	close(fd_sock);
	return 0;
}
void *listener(void *arg) {
	long fd_sock=(long)arg;

	char pathdir[UNIX_PATH_MAX+1+strlen("/data")];
	int ret;
	if(getcwd(pathdir, UNIX_PATH_MAX)==NULL) {
		perror("Error creating directory for client");
		return NULL;
	} 
	strncat(pathdir, "/data", strlen("/data"));
	if((ret=chdir(pathdir))==-1) {
		perror("Error creating directory");
		return NULL;
	}

	while(server_online==1) {
		long fd_newconn;
acc:	//accepting new connection
		if(server_online==1) {
			if((fd_newconn=accept(fd_sock, (struct sockaddr*)NULL, NULL))==-1) {
				if(errno==EAGAIN) goto acc;
				perror("Error accepting new connection");
				continue;
			}
		}
		else break;
		//create new detached thread for the new connection
		pthread_t worktid;
		pthread_attr_t attr;
		if((pthread_attr_init(&attr))!=0) {
			perror("Error initializing attr");
			close(fd_newconn);
			continue;
		}
		if((pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))!=0) {
			perror("Error setting attribute");
			pthread_attr_destroy(&attr);
			close(fd_newconn);
			continue;
		}
		if((pthread_create(&worktid, &attr, worker, (void*)fd_newconn))!=0) {
			perror("Error creating thread worker");
			pthread_attr_destroy(&attr);
			close(fd_newconn);
			continue;
		}
	}
	return (void*)0;
}

void *worker(void *arg) {
	int ret;
	long fd_newconn=(long)arg;

	//REGISTRATION
	char *endptr=NULL;
	char *token=NULL;
	char *c_id=NULL;

	//COMMUNICATION
	while(server_online==1) {
		char string[MAXMSG_SIZE+1];
		memset(string, 0, MAXMSG_SIZE+1);
		if((ret=readn(fd_newconn, string, MAXMSG_SIZE*sizeof(char)))==-1) {
			perror("Error reading request");
			writen(fd_newconn, "KO \n", 4);
			continue;
		}
		if(ret==0) continue;
		token=strtok_r(string, " ", &endptr);
		//if request is register
		if(memcmp(token, "REGISTER", strlen("REGISTER"))==0) {
			token=strtok_r(NULL, " ", &endptr);
			c_id=(char*)malloc((strlen(token)+1)*sizeof(char));
			memset(c_id, 0, strlen(token)+1);
			sprintf(c_id, "%s", token);
			if(reg(fd_newconn, token)==-1) writen(fd_newconn, "KO registration failed \n", strlen("KO registration failed \n"));
			token=NULL;
			continue;
		}
		//if request is store
		else if(memcmp(token, "STORE", strlen("STORE"))==0) {
			token=NULL;
			token=strtok_r(NULL, " ", &endptr);
			char name[strlen(token)+1];
			sprintf(name, "%s", token);
			token=NULL;
			token=strtok_r(NULL, " ", &endptr);
			char size[strlen(token)+1];
			sprintf(size, "%s", token);
			token=NULL;
			token=strtok_r(NULL, " ", &endptr);
			token=NULL;
			token=strtok_r(NULL, "\0", &endptr);			
			char data[strlen(token)+1];
			memset(data, 0, strlen(token)+1);
			sprintf(data, "%s", token);
			token=NULL;
			endptr=NULL;
			if((ret=mem(fd_newconn, name, size, data, c_id))==-1) {
				perror("Error storing data");
				writen(fd_newconn, "KO \n", 4);
			}
			token=NULL;
			continue;
		}
		//if request is delete
		else if(memcmp(token, "DELETE", strlen("DELETE"))==0) {
			token=strtok_r(NULL, " ", &endptr);
			if((ret=del(fd_newconn, token, c_id))==-1) {
				perror("Error deleting data");
				writen(fd_newconn, "KO \n", 4);
			}
			token=NULL;
			continue;
		}
		//if request si retrive
		else if(memcmp(token, "RETRIEVE", strlen("RETRIEVE"))==0) {
			token=strtok_r(NULL, " ", &endptr);
			if((ret=retr(fd_newconn, token, c_id))==-1) {
				perror("Error retrieving file");
				writen(fd_newconn, "KO \n", 4);
			}
			token=NULL;
			continue;
		}
		//if request is leave
		else if(memcmp(token, "LEAVE", strlen("LEAVE"))==0) {
			leave(c_id);
			writen(fd_newconn, "OK \n", 4);
			token=NULL;
			break;
		}

		//unkwown request
		else {
			writen(fd_newconn, "KO \n", 4);
			token=NULL;
			continue;
		}
	}
	free(c_id);
	close(fd_newconn);
	return (void*)0;
}