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

#include "communication.hpp"
#include "config.h"
#include "logger.hpp"
#include "protocol.hpp"
#include "server.hpp"
#include "socket.hpp"

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define COM_PACKET_MAX_LENGTH PROTO_DATA_MAX_LENGTH
#define COM_MAX_INCOMING_MESSAGES 1024
#define PJON_ID 0x42
#define UPDATE_PERIOD 4'000 // in us

int main()
{
	/* LOGGER */
	FILE* log_outputs[1] = {stdout};
	log_init(1, log_outputs);
	log_set_level(1); // only warnings and errors

	/* COMMUNICATION */
	if (!com_init(PJON_ID, SERIAL_DEVICE, BAUDRATE)) {
		log_error(nullptr, "Socket inititalization failure, exiting");
		return EXIT_FAILURE;
	}
	com_set_time_period(1, 1.4);
	com_set_max_attempts(40);

	/* SOCKET */
	if (!socket_init("/tmp/PJON.sock", 1024)){
		log_error(nullptr, "Socket inititalization failure, exiting");
		return EXIT_FAILURE;
	}

	/* SERVER */
	server_init(UPDATE_PERIOD);
	server_run();
}

