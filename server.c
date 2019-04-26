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

#define SOCKET_FILE "/tmp/PJON.sock"
#define MSG_LENGTH 2048
#define MAX_CONNECTION 1

int sock;

int open_socket(const char *filename)
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

	return sock;
}

int read_socket(int sock)
{
	char buffer[MSG_LENGTH];
	int nbytes;

	nbytes = read(sock, buffer, MSG_LENGTH);
	if (nbytes < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	} else if (nbytes == 0) {
		return -1;
	} else {
		fprintf(stderr, "Server: got message: `%s'\n", buffer);
		return 0;
	}
}

int main()
{
	sock = open_socket(SOCKET_FILE);
	if (listen(sock, MAX_CONNECTION) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	fd_set active_fd_set;
	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	struct sockaddr_in clientname;
	while (1) {
		fd_set read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == sock) {
					int new;
					socklen_t size = sizeof(clientname);
					new = accept(sock, (struct sockaddr *) &clientname, &size);
					if (new < 0) {
						perror("accept");
						exit(EXIT_FAILURE);
					}
					fprintf(stderr, "Server: connect from host %s, port %hd.\n",
						    inet_ntoa (clientname.sin_addr),
						    ntohs (clientname.sin_port));
					FD_SET(new, &active_fd_set);
				} else {
					if (read_socket(i) < 0) {
						close(i);
						FD_CLR(i, &active_fd_set);
					}
				}
			}
		}
}

	// This will never be called, we should use signal() to catch SIGTERM
	if (close(sock) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
