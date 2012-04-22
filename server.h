#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>


void *filladdrinfos(struct addrinfo *);

int sendf(int, char *, int); // Already an implementation in socket.h

int runserv(int, int, struct sockaddr_storage, char *, char *, long);

int makeserv(struct addrinfo *);

int server(char *);