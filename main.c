/*
Usage:
transfer -s "path/to/file"
transfer -c "HOST IP"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "client.h"

#define USAGE "Usage is:\ntransfer -c 'HOST IP'\ntransfer -s 'path/to/file'\n"

int main(int argc, char *argv[])
{
	if (argc != 3){
		fprintf(stderr, USAGE);
		exit(1);
	}

	char *option = (argv[1]+1);
	char *str_arg = argv[2];

	if (*option == 'c'){
		printf("Client mode.\n");
		client(str_arg);
	}
	else if (*option == 's'){
		printf("Server mode.\n");
		server(str_arg);
	}
	else{
		printf(USAGE);
		exit(2);
	}

	return 0;
}