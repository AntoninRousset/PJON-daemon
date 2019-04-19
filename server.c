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

#define MSG_LENGTH 2048

int open_socket(const char *filename)
{
	struct sockaddr_un name;
	int sock;
	size_t size;

	sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	name.sun_family = AF_LOCAL;
	strncpy(name.sun_path, filename, sizeof (name.sun_path));
	name.sun_path[sizeof(name.sun_path) - 1] = '\0';

	size = SUN_LEN(&name);

	if (bind(sock, (struct sockaddr *) &name, size) < 0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	return sock;
}

int read_from_client(int sock)
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
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	size_t size;

	sock = open_socket("/tmp/pjon.sock");
	if (listen(sock, 1) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	while (1) {
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == sock) {
					int new;
					size = sizeof(clientname);
					new = accept(sock, (struct sockaddr *) &clientname, &size);
					if (new < 0) {
						perror("accept");
						exit(EXIT_FAILURE);
					}
					fprintf(stderr,
						   "Server: connect from host %s, port %hd.\n",
						   inet_ntoa (clientname.sin_addr),
						   ntohs (clientname.sin_port));
					FD_SET(new, &active_fd_set);
				} else {
					if (read_from_client(i) < 0) {
						close(i);
						FD_CLR(i, &active_fd_set);
					}
				}
			}
		}
	}
}

