#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

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

#define UNIX_PATH_MAX 108
#define MAXMSG_SIZE 128
#define MAXLEN_NAME 32
#define SOCKETNAME "./objstore.sock"

//struttura dati per memorizzare i client registrati
typedef struct conn {
	char *client_id;
	struct conn *next;
} client_queue;

//variabile lock
pthread_mutex_t lock;
pthread_mutex_t lock_varglobal;

//coda globale
client_queue *head;

//variabili globali
volatile sig_atomic_t server_online;
volatile sig_atomic_t client_conn;
volatile sig_atomic_t numogg;
volatile sig_atomic_t sizeobj_store;

client_queue *insert(char *name, int *status);
client_queue *delet(char *name, int *status);
int reg(long fd, char *name);
struct dirent *search(char *path, char *name);
int mem(long fd, char *name, char* size, char *data, char *client);
int del(long fd, char *name, char *client);
int retr(long fd, char *name, char *client);
int leave(char *name);

#endif