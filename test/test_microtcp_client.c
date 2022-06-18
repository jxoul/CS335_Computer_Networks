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
 * You can use this file to write a test microTCP client.
 * This file is already inserted at the build system.
 */

#include "../lib/microtcp.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>




int main(int argc, const char * argv[]) {
    microtcp_sock_t sock;
    sock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    if(sock.state == INVALID) {
        perror("opening tcp socket");
        exit(EXIT_FAILURE);
    }
printf("CLIENT SOCKET OPEN\n");
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(14600);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if(microtcp_connect(&sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) == -1) {
        perror("tcp connect");
        exit(EXIT_FAILURE);
    }
	printf("SINDETHIKA\n");
    microtcp_shutdown(&sock,  SHUT_RDWR);
        
    
    return 0;
}
