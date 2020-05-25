CC =  gcc
AR =  ar
CFLAGS += -Wall -g -std=c99 -pedantic -Wmissing-field-initializers -D_POSIX_C_SOURCE=200809L -fsanitize=address -fno-omit-frame-pointer
ARFLAGS = rvs
LDFLAGS = -L.
LIBS = -lpthread

SHELL = /bin/bash

TARGETS = arch.o connection.o server.out client.out libclient.a libserver.a testout.log data clients

.PHONY: all clean test
.SUFFIXES: .c .h

server.out client.out : server.c libserver.a client.c libclient.a
	$(CC) $(CFLAGS) server.c -o server.out $(LDFLAGS) libserver.a $(LIBS)

	$(CC) $(CFLAGS) client.c -o client.out $(LDFLAGS) libclient.a

libserver.a libclient.a : arch.o connection.o 
	$(AR) $(ARFLAGS) libserver.a arch.o

	$(AR) $(ARFLAGS) libclient.a connection.o

arch.o connection.o : arch.c arch.h connection.c connection.h utils.h
	$(CC) $(CFLAGS) arch.c -c -o arch.o

	$(CC) $(CFLAGS) connection.c -c -o connection.o

all	: $(TARGETS)

clean : 
	rm -rf $(TARGETS)

test :
	@chmod +x test.sh

	@echo "--AVVIO TEST SCRIPT...--"

	@./test.sh

	@echo "--AVVIO SUMMARY TEST...--"

	@chmod +x testsum.sh

	@./testsum.sh testout.log

	@wait

	@rm -f testout.log