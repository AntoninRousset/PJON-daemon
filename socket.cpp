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

#include "socket.hpp"
#include "logger.hpp"
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
#include <queue>

class InputBuffer {

  public:

    InputBuffer()
    {
      this->start = 0;
      this->stop = 0;
    }

    bool ready()
    {
      return (this->stop - this->start) >= PROTO_PACKET_SIZE;
    }

    bool get(proto_packet *p)
    {
      if (!this->ready())
        return false;
      memcpy(p, &this->data[this->start], PROTO_PACKET_SIZE);
      this->start += PROTO_PACKET_SIZE;
      return true;
    }

    ssize_t read_file(int fd)
    {
      if (this->start != 0) {
        memmove(this->data, &this->data[this->start], this->stop - this->start);
        this->stop = this->stop - this->start;
        this->start = 0;
      }
      ssize_t count = read(fd, &this->data[this->stop],
          SOCKET_INPUT_BUFFER_SIZE - this->stop);
      if (count > 0)
        this->stop += count;

      return count;
    }

  private:

    unsigned int start, stop;
    char data[SOCKET_INPUT_BUFFER_SIZE];

};

class OutputQueue: public std::queue<proto_packet> {

  public:

    OutputQueue(): std::queue<proto_packet>()
  {}

    void clear()
    {
      while (!this->empty())
        this->pop();
    }

}; 

static int master_socket = -1;
static unsigned int max_clients;
static fd_set active_fds, read_fds, write_fds; 
static std::vector<InputBuffer> input_buffers;
static std::vector<OutputQueue> output_queues;

static int open_socket(const char* filename);
static bool can_read(int sock);
static bool can_write(int sock);
static int accept_slave();
static void close_slave(int sock);

bool socket_init(const char *fp, unsigned int mc)
{
  log_info("socket", "openning master socket");
	master_socket = open_socket(fp);
  max_clients = mc;
  input_buffers.resize(mc);
  output_queues.resize(mc);
  FD_ZERO(&active_fds);
  FD_SET(master_socket, &active_fds);
  return master_socket < 0 ? false : true;
}

bool socket_wait(unsigned int timeout)
{
  read_fds = active_fds;
  write_fds = active_fds;

  struct timeval tv = {0, timeout};
  if (select(FD_SETSIZE, &read_fds, NULL, NULL, &tv) < 0) {
    log_perror("select read socket", "select");
    return false;
  }

  tv.tv_usec = 0;
  if (select(FD_SETSIZE, NULL, &write_fds, NULL, &tv) < 0) {
    log_perror("select write socket", "select");
    return false;
  }

  return true;
}

std::vector<proto_packet> socket_receive(int sock)
{
  proto_packet p;
  std::vector<proto_packet> packets;

  // not readable
  if (!can_read(sock))
    return packets;

  // new
  if (sock == master_socket) {
    accept_slave();
    return packets; //TODO check need to return -> I think so
  }

  ssize_t count = input_buffers[sock].read_file(sock);

  // reading error
	if (count < 0)
    log_perror("socket", "Read socket");
  
  // end of file or error -> closing
  if (count <= 0) {
    close_slave(sock);
    return packets;
  }

  // get packets
  while(input_buffers[sock].get(&p))
    packets.push_back(p);

  return packets;
}

void socket_push(int sock, proto_packet p)
{
  if (sock != SOCKET_ALL) {
    output_queues[sock].push(p);
    return;
  }

  //TODO can be done better? 
  int max_clients = socket_get_max_clients();
  for (int sock = 0; sock < max_clients; sock++) {
    if (FD_ISSET(sock, &active_fds) && sock != master_socket)
      socket_push(sock, p);
  }

}

int socket_send(int sock)
{
  if (!can_write(sock))
    return 0;

  auto& q = output_queues[sock];
  unsigned int n = 0;

  while (!q.empty()) {
    ssize_t count = write(sock, &q.front(), sizeof(proto_packet));
    if (count == sizeof(proto_packet))
      q.pop();
    n++;
  }

  return n;
}

unsigned int socket_get_max_clients()
{
  return max_clients;
}

bool socket_quit()
{
  return true;
}

int open_socket(const char* filename)
{
	int sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
    return -1;
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
    return -1;
	}

	if (listen(sock, 10000) < 0) {
		perror("listen");
    return -1;
	}

	return sock;
}

bool can_read(int sock)
{
  return FD_ISSET(sock, &read_fds);
}

bool can_write(int sock)
{
  return FD_ISSET(sock, &write_fds);
}

int accept_slave()
{
	struct sockaddr_in client;
	socklen_t size = sizeof(client);
	int slave = accept(master_socket, (struct sockaddr*) &client, &size);
	if (slave < 0) {
		log_perror("socket", "Accept slave");
  } else {
    FD_SET(slave, &active_fds);
    log_info("socket", "New slave %d", slave);
  }

  // be sure the output queue is empty
  output_queues[slave].clear();

  // send version packet
  proto_packet p;
  proto_new_packetVersion((proto_packetVersion*) &p, PROTO_VERSION);
  socket_push(slave, p);
	return slave;
}

void close_slave(int sock)
{
  log_info("socket", "Remove slave %d", sock);
  close(sock);
  FD_CLR(sock, &active_fds);
  // empty output queue
  output_queues[sock].clear();
}

