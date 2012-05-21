/*
	client.c

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

#include "client.h"

const int port = 1234;
const int kb_size = 1024;
extern int errno; // Used for general error codes

int client(char *host_ip)
{
	struct addrinfo *servinfo = 0;
	int sockfd = 0;

	int connect_status = 0;

	servinfo = filladdrinfoc(host_ip, servinfo);
/*
	Open socket (domain, type, protocol) 
*/	
	if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol))==-1){
		fprintf(stderr, "Socket error: %i\n", errno);
		exit(5);
	}

	if((connect_status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==0){
		printf("Connected on port: %i \n",port);
		runclient(sockfd);
	}
	else{
		fprintf(stderr, "%i, %i\n", connect_status, errno);
	}

	freeaddrinfo(servinfo);

	return 0;
}

void *filladdrinfoc(char *host_ip, struct addrinfo *servinfo){
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
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
/*
	Make a call to getaddrinfo with partially filled struct hints, ip, port
	and struct to fill with addrinfo.
*/	
	if((status = getaddrinfo(host_ip, port_a, &hints, &servinfo))){
		fprintf(stderr, "Addrinfo error: %s\n", gai_strerror(status));
	}
	return servinfo;
}

long recvf(int sockfd, long filesize, char *rec_buffer, char *file_buffer){
	long dreclentot = 0;
	int dreclen = 0;
	long remaining_bytes = 0;
	int packet = 0;
	int i = 0;

	while(dreclentot!=filesize){
		// Write to rec buffer to account for TCP, then write all to file_buffer

		if((dreclen = recv(sockfd, rec_buffer, kb_size, 0))==-1){
			fprintf(stderr, "Receive error: %i\n", errno);
		}
		else if (!dreclen){
			fprintf(stderr, "Host closed the connection.\n");
			exit(5);
		}

		dreclentot += dreclen;
		packet++;
		remaining_bytes = (filesize - dreclentot);
		printf("%g%%\n", ((float)dreclentot/(float)filesize) * 100);

		for (i = 0; i < dreclen; ++i){
			file_buffer[(dreclentot-dreclen)+i] = rec_buffer[i]; // copy recieved this iteration to final file buffer
		}
	}
	printf("Received %i packets.\n", packet);

	printf("Received %li bytes, %g%%\n",dreclentot, ((float)dreclentot/(float)filesize)*100);

	return dreclentot;
}

int runclient(int sockfd){
	long filesize = 0;
	char *filename;
	char reqmsg[100];
	char response[2];
	long dreclentot = 0;

	char *rec_buffer;
	char *file_buffer; 

	FILE *file;

	recv(sockfd, reqmsg, sizeof(reqmsg), 0);

	filename = strtok(reqmsg, ":"); // Split on ":"
	filesize = atoi(strtok(NULL, ":"));

	printf("Accept file transfer request: '%s', (%li)? (Y/N)\n", filename, filesize);

	response[0] = (char)toupper(getchar());

	if(send(sockfd, &response, sizeof response, 0)==-1){
		fprintf(stderr, "Send error: %i\n", errno);
	}

	if (strncmp(&response[0], "Y", 1)==0){
		if((file = fopen(filename, "wb"))==NULL){
			fprintf(stderr, "File Opening Error.\n");
			exit(2);
		}
		if((rec_buffer = malloc(kb_size))==NULL){
			fprintf(stderr, "Receive buffer malloc failed.\n");
			exit(3);
		}
		if((file_buffer = malloc(filesize))==NULL){
			fprintf(stderr, "File buffer malloc failed.\n");
			exit(4);
		}

		dreclentot = recvf(sockfd, filesize, rec_buffer, file_buffer);

		if (dreclentot!=filesize){
			fprintf(stderr, "Warning: Not all bytes received.\n");
		}
		else{
			fwrite (file_buffer , 1 , filesize, file); // This MUST be the same size of the original file
			printf("File transfer completed, closing file....\n");
		}
			
		free(rec_buffer);
		free(file_buffer);
			
		fclose(file);
	}
	else {
		printf("File transfer rejected.\n");
	}
	close(sockfd);

	return 0;
}