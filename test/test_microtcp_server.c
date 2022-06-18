/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */
#include <stdio.h>
#include "../lib/microtcp.h"
#include <stdlib.h>
#include <string.h>
#include "../utils/crc32.h"
#include <time.h>
#include <netinet/in.h>

int main(int argc, const char * argv[]) {
    microtcp_sock_t socket;
    int accepted;
    struct sockaddr_in sin;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    
    socket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    if (socket.state == INVALID) {
        perror("opening TCP socket");
        exit(EXIT_FAILURE);
    }
    printf("opening socket ok\n");
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(14600);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (microtcp_bind(&socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == -1){
        perror("TCP bind");
        exit(EXIT_FAILURE);
    }

    	printf("Bind OK\n");
	//socket.state = LISTEN;
    	printf("Listen OK\n");
    	client_addr_len = sizeof(struct sockaddr);


    	while ((accepted = microtcp_accept(&socket,&client_addr, client_addr_len)) > 0){
        	printf("New connection accepted!\n");       
		microtcp_shutdown(&socket,  SHUT_RDWR);
	}
    return 1;
}
