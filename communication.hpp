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


//TODO implement get_ping() and get_success_rate() and warnings
//TODO implement warning "filling up queue"

#pragma once

#include <stdint.h>
#include <string.h>

#ifndef COM_PACKET_MAX_LENGTH
#define COM_PACKET_MAX_LENGTH 50
#endif
#define PJON_PACKET_MAX_LENGTH COM_PACKET_MAX_LENGTH

#ifndef COM_MAX_INCOMING_MESSAGES
#define COM_MAX_INCOMING_MESSAGES 1024
#endif

#ifndef COM_OUTGOING_QUEUE_WARNING_THRESHOLD
#define COM_OUTGOING_QUEUE_WARNING_THRESHOLD 32
#endif

#ifndef COM_PING_WARNING_THRESHOLD
#define COM_PING_WARNING_THRESHOLD 30'000 // in us
#endif

#ifndef COM_SUCCESS_RATE_WARNING_THRESHOLD
#define COM_SUCCESS_RATE_WARNING_THRESHOLD 0.95
#endif

typedef uint8_t com_id;
typedef int16_t com_ref;

enum com_state : int8_t {
  COM_PENDING = 0,
  COM_SUCCESS = 1,
  COM_FAILED_OPEN_SERIAL  = -1,
  COM_CONTENT_TOO_LONG    = -2, 
  COM_CONNECTION_LOST     = -3
};

typedef struct {
  com_ref ref;
  enum com_state state;
} com_request;

typedef struct {
  com_id src;
  size_t n;
  char data[PJON_PACKET_MAX_LENGTH];
} com_message;


// Initiate communication with the given id. The device dev is used
// with a baudrate bd for serial communication.
// id: PJON id
// dev: serial device path (should be in /dev/)
// bd: serial baudrate
// Return true in case of success, false otherwise
bool com_init(com_id id, const char *dev, uint32_t bd);

// Set maximum of attempts m for a outgoing packet before COM_CONNECTION_LOST
// is returned.
void com_set_max_attempts(unsigned int m);

// Set the initial minimum period t0 in us between two dispatch trial and the
// logarithmic factor f. At each each dispatch trial, the period of the packet
// is multiplied by f until the maximum attempts number is reached or success.
void com_set_time_period(float t0, float f);

// Connect to serial device given at initialization.
// Return true in case of successfull connection, false otherwise
bool com_connect();

// Return true in case of successfull connection, false otherwise
bool com_is_connected();

// Add with the reference r, the data of n bytes to the outgoing stack to be
// send to dest at the next com_send call
// r: reference of the request, is returned by com_send
// dest: PJON id of the destination
// n: size in bytes of the data
// data: raw data to be sent
// return true in case of success, false otherwise (e.g. stack is full)
bool com_push(com_ref r, com_id dest, size_t n, const void* data);

// Cancel the request given by the reference r
void com_cancel(com_ref r);

// Try to sends the packet in the outgoing stack. Fill results with the state
// of finished requests with their reference. The states may be COM_SUCCESS,
// COM_FAILED_OPEN_SERIAL, COM_CONTENT_TOO_LONG or COM_CONNECTION_LOST.
// results: a n_max long array to be filled with the finished results
// n_max: maximum number of finished requests, no request are lost if full
// return the number of finished requests (a.k.a. the number of element to
// read in results)
size_t com_send(com_request *results, size_t n_max);

// Try to receive messages. Fill reception with the received messages.
// reception: a n_max long array to be filled with the received messages
// n_max: the maximum number of received messages, if full some received 
// messages may be lost so take a large number
// return the number of received messages (a.k.a. the number of element to
// read in reception)
size_t com_receive(com_message *reception, size_t n_max);

