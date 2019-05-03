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

#include "protocol.hpp"

#define COM_PACKET_MAX_LENGTH PROTO_DATA_MAX_LENGTH
#define COM_MAX_INCOMING_MESSAGES 1024
#define PJON_ID 0x42
#define SERIAL_DEVICE "/dev/escaperoom"
#define BAUDRATE 19200

#include "logger.hpp"
#include "communication.hpp"
#include "socket.hpp"
#include "server.hpp"
#include <stdlib.h>

int main()
{
  /* LOGGER */
  FILE* log_outputs[1] = {stdout};
  log_init(1, log_outputs);

  /* COMMUNICATION */
  if (!com_init(PJON_ID, SERIAL_DEVICE, BAUDRATE)) {
    log_error(nullptr, "Socket inititalization failure, exiting");
    return EXIT_FAILURE;
  }

  /* SOCKET */
  if (!socket_init("/tmp/PJON.sock")){
    log_error(nullptr, "Socket inititalization failure, exiting");
    return EXIT_FAILURE;
  }

  /* SERVER */
  server_init(1'000'000);
  server_run();
}

