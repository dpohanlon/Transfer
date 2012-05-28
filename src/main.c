/*
	main.c

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
