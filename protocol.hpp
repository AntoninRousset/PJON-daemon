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

//TODO ensure little_endian

#pragma once

#include <stdint.h>

#include "config.h"

#define PROTO_VERSION "0.0.1"
#define PROTO_PACKET_SIZE 64
#define PROTO_DATA_MAX_LENGTH 50

typedef uint8_t proto_head;
typedef uint8_t proto_id;
typedef uint16_t proto_dataLength;
typedef uint16_t proto_code;
typedef uint16_t proto_outgoingResult;
typedef char proto_data;

#pragma pack(push, 1)

typedef struct {
	proto_head head;
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)];
} proto_packet;

typedef struct {
	proto_head head;
	char version[PROTO_PACKET_SIZE-sizeof(proto_head)];
} proto_packetVersion;

typedef struct {
	proto_head head;
	proto_code code;
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)-sizeof(proto_code)];
} proto_packetInfo;

typedef struct {
	proto_head head;
	proto_code code;
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)-sizeof(proto_code)];
} proto_packetWarn;

typedef struct {
	proto_head head;
	proto_code code;
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)-sizeof(proto_code)];
} proto_packetError;

typedef struct {
	proto_head head;
	proto_id src;
	proto_dataLength length;
	proto_data data[PROTO_DATA_MAX_LENGTH];
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)-sizeof(proto_id)
		-sizeof(proto_dataLength)-PROTO_DATA_MAX_LENGTH*sizeof(proto_data)];
} proto_packetIngoingMessage;

typedef struct {
	proto_head head;
	proto_id dest;
	proto_dataLength length;
	proto_data data[PROTO_DATA_MAX_LENGTH];
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)-sizeof(proto_id)
		-sizeof(proto_dataLength)-PROTO_DATA_MAX_LENGTH*sizeof(proto_data)];
} proto_packetOutgoingMessage;

typedef struct {
	proto_head head;
	proto_outgoingResult result;
	char padding[PROTO_PACKET_SIZE-sizeof(proto_head)-sizeof(proto_outgoingResult)];
} proto_packetOutgoingResult;


#pragma pack(pop)

static_assert(sizeof(proto_packet) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packet");
static_assert(sizeof(proto_packetVersion) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetVersion");
static_assert(sizeof(proto_packetInfo) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetInfo");
static_assert(sizeof(proto_packetWarn) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetWarn");
static_assert(sizeof(proto_packetError) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetError");
static_assert(sizeof(proto_packetIngoingMessage) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetIngoingMessage");
static_assert(sizeof(proto_packetOutgoingMessage) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetOutgoingMessage");
static_assert(sizeof(proto_packetOutgoingResult) == PROTO_PACKET_SIZE,
		"Invalid struct proto_packetOutgoingResult");

/* Heads */
#define PROTO_HEAD_VERSION          0x00
#define PROTO_HEAD_INFO             0x01
#define PROTO_HEAD_WARN             0x02
#define PROTO_HEAD_ERROR            0x03
#define PROTO_HEAD_INGOING_MSG      0x04
#define PROTO_HEAD_OUTGOING_MSG     0x05
#define PROTO_HEAD_OUTGOING_RESULT  0x06

#define PROTO_INFO_SERIAL_OPENED  0x01

#define PROTO_ERROR_FAILED_OPEN_SERIAL            0x01
#define PROTO_ERROR_RECEIVED_INVALID_PACKET_HEAD  0x02

#define PROTO_OUTGOING_RESULT_SUCCESS             0x00
#define PROTO_OUTGOING_RESULT_INTERNAL_ERROR      0x01
#define PROTO_OUTGOING_RESULT_CONTENT_TOO_LONG    0x02
#define PROTO_OUTGOING_RESULT_CONNECTION_LOST     0x03

proto_packet proto_read_copy(const char *buffer);
proto_head proto_read_head(const char *buffer);

bool proto_new_packet(proto_packet *p, proto_head head);
bool proto_new_packetVersion(proto_packetVersion *p, const char* version);
bool proto_new_packetInfo(proto_packetInfo *p, proto_code code);
bool proto_new_packetWarn(proto_packetWarn *p, proto_code code);
bool proto_new_packetError(proto_packetError *p, proto_code code);
bool proto_new_packetIngoingMessage(proto_packetIngoingMessage *p,
				 proto_id src, proto_dataLength length, const proto_data* data);
bool proto_new_packetOutgoingMessage(proto_packetOutgoingMessage *p,
				proto_id dest, proto_dataLength length, const proto_data* data);
bool proto_new_packetOutgoingResult(proto_packetOutgoingResult *p,
									proto_outgoingResult result);


