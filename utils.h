#ifndef	UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    const char c='\n';
    while(left>0) {
    if ((r=read((int)fd ,bufptr,left)) == -1) {
        if (errno == EINTR) continue;
        return -1;
    }
    if(strchr(bufptr, c)!=NULL) {
            left    -= r;
            bufptr  += r;
            break;
        }
    if (r == 0) return 0;   // gestione chiusura socket
    left    -= r;
    bufptr  += r;
    }
    return size;
}

static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
    if ((r=write((int)fd ,bufptr,left)) == -1) {
        if (errno == EINTR) continue;
        return -1;
    }
    if (r == 0) return 0;  
    left    -= r;
    bufptr  += r;
    }
    return 1;
}

#endif