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

#define MAXMSG  512

int open_socket(const char *filename)
{
  struct sockaddr_un name;
  int sock;
  socklen_t size;

  /* Create the socket. */
  sock = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  /* Bind a name to the socket. */
  name.sun_family = AF_LOCAL;
  strncpy (name.sun_path, filename, sizeof (name.sun_path));
  name.sun_path[sizeof (name.sun_path) - 1] = '\0';

  size = SUN_LEN(&name);

  if (connect(sock, (struct sockaddr *) &name, size) < 0)
    {
      perror ("connect");
      exit (EXIT_FAILURE);
    }

  return sock;
}

int write_to_server(int sock, const char* str)
{
  ssize_t nbytes = send(sock, str, strlen(str), 0);
  if (nbytes < 0)
    perror("Send");
  return nbytes;
}

int main()
{
  int sock = open_socket("/tmp/pjon.sock");
  if (sock < 0)
    return EXIT_FAILURE;
  write_to_server(sock, "Hello world");
  return EXIT_SUCCESS;
}

