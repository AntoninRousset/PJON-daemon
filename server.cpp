/* 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <signal.h>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "communication.hpp"

#define SOCKET_FILE "/tmp/PJON.sock"
#define BUFFER_SIZE 2048
#define MAX_CONNECTION 1
#define UPDATE_PERIOD 20000

#define BAUDRATE 19200
#define ID_COMPUTER 0x42
#define ID_UNO	0x22
#define ID_NANO 0x33

int open_socket(const char* filename)
{
	int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_un name;
	memset(&name, 'x', sizeof(name));
	name.sun_family = AF_LOCAL;
	name.sun_path[0] = '\0';
	strncpy(name.sun_path+1, filename, strlen(filename));
	socklen_t size = offsetof(struct sockaddr_un, sun_path)
				   + strlen(filename) + 1;
	if (bind(sock, (struct sockaddr*) &name, size) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sock, MAX_CONNECTION) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	return sock;
}

int accept_slave(int master)
{
	struct sockaddr_in client;
	socklen_t size = sizeof(client);
	int slave = accept(master, (struct sockaddr*) &client, &size);
	if (slave < 0)
		perror("Accept slave");
	return slave;
}

int read_socket(int sock)
{
	char buffer[BUFFER_SIZE];
	size_t count = read(sock, buffer, sizeof(buffer));
	if (count < 0) {
		perror("Read socket");
		exit(EXIT_FAILURE);
	} else if (count == 0) {
		return -1;
	} else {
		printf("message from %d: `%s'\n", sock, buffer);
		// Forward to PJON
		// .push(sock, 0x0000, count, buffer);
		return 0;
	}
}

int main()
{
  printf("Openning master socket\n");
	int master = open_socket(SOCKET_FILE);

  Communication com(ID_COMPUTER);

	fd_set active_fds;
	FD_ZERO(&active_fds);
	FD_SET(master, &active_fds);
    printf("Running %d\n");
	while (1) {
		fd_set read_fds = active_fds;
		struct timeval tv = {0, UPDATE_PERIOD};
		if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == master) {
					int slave = accept_slave(master);
					if (slave >= 0) {
						FD_SET(slave, &active_fds);
						fprintf(stderr, "new slave %d.\n", slave);
					}
				} else {
					if (read_socket(i) < 0) {
						printf("rem slave %d.\n", i);
						close(i);
						FD_CLR(i, &active_fds);
					}
				}
			}
		}
		// PJON update
	}
	return EXIT_SUCCESS;
}
