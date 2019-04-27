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

#define SOCKET_FILE "/tmp/PJON.sock"
#define MSG_LENGTH 2048

int open_socket(const char *filename)
{
	int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_un name;
	name.sun_family = AF_LOCAL;
	strncpy(name.sun_path, filename, sizeof (name.sun_path));
	name.sun_path[sizeof(name.sun_path) - 1] = '\0';
	socklen_t size = SUN_LEN(&name);
	if (connect(sock, (struct sockaddr *) &name, size) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	return sock;
}

int write_socket(int sock, const void* data, size_t size)
{
	ssize_t nbytes = send(sock, data, size, 0);
	if (nbytes < 0)
		perror("Send");

	return nbytes;
}

int main()
{
	int sock = open_socket(SOCKET_FILE);
	if (sock < 0)
		return EXIT_FAILURE;

	const char* msg = "Hello world";
	write_socket(sock, msg, strlen(msg));

	shutdown(sock, 0);
	return EXIT_SUCCESS;
}

