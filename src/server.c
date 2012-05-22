/*
	server.c

	Copyright (C) 2012 Daniel O'Hanlon	

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
 
#include "server.h"

const int backlog = 5; // Max 5 set by kernel ?
const int opttrue = 1;

extern int errno;
extern int port;
extern int kb_size;

/*
int clientfd, int client, struct sockaddr_storage client_addr, 
char *buffer, char *filename, long lSize
*/

struct server_data
{
	int clientfd;
	int client;
	struct sockaddr_storage client_addr;
	char *buffer;
	char *filename;
	long lSize;
	int thread_flag;
};

int server(char *filename)
{
	struct addrinfo *servinfo = 0;

	struct sockaddr_storage client_addr;
	socklen_t addr_size = sizeof client_addr;

	struct server_data thread_arg;

	int sockfd = 0;
	int clientfd = 0;
	int client = 0;

	pthread_t thread;

	long lSize = 0;
  	char * buffer;

	FILE *file;
	if((file = fopen(filename, "rb"))==NULL){
		fprintf(stderr, "File Opening Error.\n");
		exit(2);
	}

	fseek (file , 0 , SEEK_END); // Find EOF
  	lSize = ftell (file);
  	rewind (file);

  	// allocate memory to contain the whole file:
  	buffer = (char*) malloc (sizeof(char)*lSize);
  	if (buffer == NULL) {
  		fprintf(stderr, "Buffer Malloc Error\n");
  		exit (3);
  	}

  	// copy the file into the buffer:
  	if (fread(buffer,1,lSize,file) != lSize) {
  		fprintf(stderr, "Buffer Fill Error\n");
  		exit (4);
  	}

  	fclose(file);

  	servinfo = filladdrinfos(servinfo);

	sockfd = makeserv(servinfo);

/*
	Listen for incoming connections on 1234	
*/
	if(listen(sockfd, backlog)==-1){
		fprintf(stderr, "Listen Error: %i\n", errno);
	}

	printf("Listening on port: %i \n",port);

	client = 0;
/*
	Fill struct with thread arguments
*/
	thread_arg.client_addr = client_addr;
	thread_arg.buffer = buffer;
	thread_arg.filename = filename;
	thread_arg.lSize = lSize;
	thread_arg.thread_flag = 0;

/*
	Accept an incoming connection	
*/	

	//while(1){
		if ((clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size))){
			client++;

			thread_arg.clientfd = clientfd;
			thread_arg.client = client;

			pthread_create(&thread, NULL, runserv, (void *)&thread_arg);

			//runserv((void *)&thread_arg);

			pthread_join(thread, NULL);

			close(clientfd);
		}
	//}
/*
	Free our servinfo, and other assorted allocated goodies
*/

	close(sockfd);
	freeaddrinfo(servinfo);
	free(buffer);

	return 0;
}

// packets?

int sendf(int clientfd, char *buffer, int lSize){
	int dsentlen = 0;
	int packet = 0;
	int remaining_bytes = 0;
	while(dsentlen!=lSize){
		if((dsentlen += send(clientfd,&buffer[packet*kb_size], kb_size, 0))==-1){
			fprintf(stderr, "Sending Error: %i\n", errno);
		}
		packet++;
		remaining_bytes = (lSize - packet*kb_size);
		if (remaining_bytes < kb_size){
			/* send the last packet  of < 1024 bytes, required */
			if((dsentlen += send(clientfd,&buffer[packet*kb_size], remaining_bytes, 0))==-1){
				fprintf(stderr, "Sending Error (last packet): %i\n", errno);
			}
			packet++;
		}
	}
	return dsentlen;
}

// client -> parallel?

void *runserv(void *arg){
	char peeripstr[INET_ADDRSTRLEN];
	char reqmsg[100] = {0};
	char lSizea[100] = {0};
	char response[1] = {0};
	int msentlen = 0;
	int dsentlen = 0;

	struct server_data *local_data;

	local_data = (struct server_data *)arg;

	int clientfd = (int)local_data->clientfd;
	int client = (int)local_data->client;
	struct sockaddr_storage client_addr = (struct sockaddr_storage)local_data->client_addr;
	char *buffer = (char *)local_data->buffer;
	char *filename = (char *)local_data->filename;
	long lSize = (long)local_data->lSize;
	int thread_flag = (int)local_data->thread_flag;

	struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
	int ipAddr = pV4Addr->sin_addr.s_addr;		
	inet_ntop(AF_INET, &ipAddr, peeripstr, sizeof peeripstr); 

	printf("Client %i connected from %s.\n",client, peeripstr);

	snprintf(lSizea, 100, "%li",lSize);

	strncat(reqmsg, filename, strlen(filename));
	strncat(reqmsg, ":", strlen(":"));
	strncat(reqmsg, lSizea, strlen(lSizea));

	printf("Requesting to send file '%s' (%li) \n", filename, lSize);

	msentlen = send(clientfd, reqmsg, sizeof reqmsg, 0);
	printf("%s\n", reqmsg);
	recv(clientfd, response, 10, 0);

	if (strncmp(&response[0], "Y", 1) == 0){
		printf("Request accepted. Sending file....\n");

		dsentlen = sendf(clientfd, buffer, lSize);

		printf("Sent %i bytes, %g%%\n",dsentlen, ((float)dsentlen/(float)lSize)*100);

		thread_flag = 0;
	}

	thread_flag = 1;

	return NULL;
}

void *filladdrinfos(struct addrinfo *servinfo){
	int status = 0;
	struct addrinfo hints;
	char port_a[8] = {0};

	snprintf(port_a, 8, "%d",port);
/*
	Make sure hints is empty before we begin
*/
	memset(&hints, 0, sizeof hints);
/*
	Fill hints with some initial 'hint' info
*/
	hints.ai_family = AF_INET; // ipv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; // use my IP
/*
	Make a call to getaddrinfo with partially filled struct hints, ip, port
	and struct to fill with addrinfo.
*/	
	if((status = getaddrinfo(NULL, port_a, &hints, &servinfo))){
		fprintf(stderr, "Addrinfo error: %s\n", gai_strerror(status));
	}

	return servinfo; // Pointer, servinfo modified
}

int makeserv(struct addrinfo *servinfo){
	int sockfd;
	if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol))==-1){
		fprintf(stderr, "Socket error: %i\n", errno);
		exit(5);
	}

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opttrue, sizeof(opttrue))==-1){
		fprintf(stderr, "Socket Option Error: %i\n", errno);
	}

	if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)==-1){
		fprintf(stderr, "Bind Error: %i\n", errno);
		exit(6);
	}
	return sockfd;
}