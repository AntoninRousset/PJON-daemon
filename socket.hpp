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

//TODO check includes in .cpp
//TODO implement socket_quit()
//TODO empty output queue if socket closes
//TODO check wrong term "stack" in other files and replace by "queue"
//TODO correctly handle max clients -> refuse connection if too much
//TODO bigger defualt SOCKET_INPUT_BUFFER_SIZE

#pragma once

#include "protocol.hpp"
#include <sys/select.h>
#include <vector>

#ifndef SOCKET_INPUT_BUFFER_SIZE 
#define SOCKET_INPUT_BUFFER_SIZE 2048
#endif

#define SOCKET_ALL -1

// Initialize the socket to the file path filepath with a maximum number of
// clients mc 
// Return false in case of failure, true otherwise
bool socket_init(const char *filepath="/tmp/PJON.sock", unsigned int mc=256);

// Wait for any socket to be readable or writable, or for the timeout to be reached
// timeout: maximum blocking time in us
// Return false in case of error, true otherwise
bool socket_wait(unsigned int timeout);

// Return new packets from the socket sock
std::vector<proto_packet> socket_receive(int sock);

// Push to the output list new packet p to be send to socket sock at next call
// of socket_send
// sock: destination socket, SOCKET_ALL can be used to send to all sockets
// Return the number of packets in the output queue for the socket sock
void socket_push(int sock, proto_packet p);

// Try to send packets pushed in the output queue for the socket sock
// Return the number of packets sent
int socket_send(int sock);

// Return the maxium number of connections (a.k.a. the number of socket to
// iterate over)
unsigned int socket_get_max_clients();

// Quit
bool socket_quit();

