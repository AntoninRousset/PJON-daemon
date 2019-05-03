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
#include <string.h>

proto_packet proto_read_copy(const char *buffer)
{
  proto_packet p;
  memcpy(&p, buffer, PROTO_PACKET_SIZE);
  return p;
}

proto_head proto_read_head(const char *buffer)
{
  return ((proto_packet*) buffer)->head;
}

bool proto_new_packet(proto_packet *p, proto_head head)
{
  p->head = head;
  return true;
}

bool proto_new_packetVersion(proto_packetVersion *p, const char* version)
{
  p->head = PROTO_HEAD_VERSION;
  p->version[0] = '\0';
  if (strlen(version)+1 > sizeof(version))
    return false;
  strncpy(p->version, version, sizeof(p->version));
  p->version[sizeof(p->version)-1] = '\0';
  return true;
}

bool proto_new_packetInfo(proto_packetInfo *p, proto_code code)
{
  p->head = PROTO_HEAD_INFO;
  p->code = code;
  return true;
}

bool proto_new_packetWarn(proto_packetWarn *p, proto_code code)
{
  p->head = PROTO_HEAD_WARN;
  p->code = code;
  return true;
}

bool proto_new_packetError(proto_packetError *p, proto_code code)
{
  p->head = PROTO_HEAD_ERROR;
  p->code = code;
  return true;
}

bool proto_new_packetIngoingMessage(proto_packetIngoingMessage *p,
    proto_id src, proto_dataLength length, const proto_data* data)
{
  p->head = PROTO_HEAD_INGOING_MSG;
  p->src = src;
  p->length = 0;
  if (length > PROTO_DATA_MAX_LENGTH)
    return false;
  p->length = length;
  memcpy(p->data, data, length);
  return true;
}

bool proto_new_packetOutgoingMessage(proto_packetOutgoingMessage *p,
    proto_id dest, proto_dataLength length, const proto_data* data)
{
  p->head = PROTO_HEAD_OUTGOING_MSG;
  p->dest = dest;
  p->length = 0;
  if (length > PROTO_DATA_MAX_LENGTH)
    return false;
  p->length = length;
  memcpy(p->data, data, length);
  return true;
}

bool proto_new_packetOutgoingResult(proto_packetOutgoingResult *p,
    proto_outgoingResult result)
{

}


