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

#pragma once

//TODO logger
//TODO remove using namespace
//TODO redo period system, -> need float if log

#include <stdint.h>
#include <string.h>

#ifndef COM_PACKET_MAX_LENGTH
#define COM_PACKET_MAX_LENGTH 50
#endif
#define PJON_PACKET_MAX_LENGTH COM_PACKET_MAX_LENGTH

#ifndef COM_MAX_INCOMING_MESSAGES
#define COM_MAX_INCOMING_MESSAGES 1024
#endif

typedef uint8_t PJON_id_t;
typedef int16_t Reference_t;

enum RequestState : int8_t {
  COM_PENDING = 0,
  COM_SUCCESS = 1,
  COM_FAILED_OPEN_SERIAL  = -1,
  COM_CONTENT_TOO_LONG    = -2, 
  COM_CONNECTION_LOST     = -3
};

typedef struct {
  Reference_t ref;
  enum RequestState state;
} Request_t;

typedef struct {
  PJON_id_t src;
  size_t n;
  char data[PJON_PACKET_MAX_LENGTH];
} Message_t;


void com_init(PJON_id_t id, unsigned int max_attempts=32, float initial_period_ms=1,
    float period_factor=1.5);

bool com_connect(const char * dev, uint32_t baudrate);

bool com_is_connected();

// return False in case of failure (e.g. stack is full)
bool com_push(Reference_t r, PJON_id_t dest, size_t n, const void* data);

void com_cancel(Reference_t r);

size_t com_send(Request_t * results, size_t n_max);

size_t com_receive(Message_t * reception, size_t n_max);

void com_quit();

