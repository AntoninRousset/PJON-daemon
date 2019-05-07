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

#include "server.hpp"
#include "logger.hpp"
#include "socket.hpp"
#include <vector>

static unsigned int update_period;

void server_init(unsigned int up)
{
  log_info("server", "Initialization");
  update_period = up;
}

void server_run()
{
  log_info("server", "Running");
  if (com_connect()) {
    proto_packet p;
    proto_new_packetInfo((proto_packetInfo*) &p, PROTO_INFO_SERIAL_OPENED);
    socket_push(SOCKET_ALL, p);
  }

  while (true) {

    if (!socket_wait(update_period))
      return;

    unsigned int max_clients = socket_get_max_clients();
    for (unsigned int sock = 0; sock < max_clients; sock++) {

      // socket reception
      std::vector<proto_packet> packets = socket_receive(sock);
      for (const proto_packet& p : packets) {
        log_packet("server",  &p, "Received from %d", sock);
        auto p1 = (proto_packetOutgoingMessage*) &p;
        if (p.head != PROTO_HEAD_OUTGOING_MSG) {
          proto_packet p_error;
          log_error("server", "Received invalid packet head (expecting : %d, "
            "received: %d)", PROTO_HEAD_INGOING_MSG, p.head);
          proto_new_packetError((proto_packetError*) &p_error,
              PROTO_ERROR_RECEIVED_INVALID_PACKET_HEAD);
          socket_push(sock, p_error);
          break;
        }
        com_push(sock, p1->dest, p1->length, p1->data);
      }

      // socket emission
      socket_send(sock);
    }

    if (!com_is_connected()) {
      proto_packet p;
      proto_new_packetError((proto_packetError*) &p,
          PROTO_ERROR_FAILED_OPEN_SERIAL);
      socket_push(SOCKET_ALL, p);
      if (com_connect()) {
        proto_packet p;
        proto_new_packetInfo((proto_packetInfo*) &p, PROTO_INFO_SERIAL_OPENED);
        socket_push(SOCKET_ALL, p);
      }
    }

    // PJON emission
    com_request results[1000];
    size_t n = com_send(results, 1000);
    for (unsigned int i = 0; i < n; i++) {
      com_request req = results[i];
      proto_packet p;
      switch (req.state) {
        case COM_SUCCESS:
          proto_new_packetOutgoingResult((proto_packetOutgoingResult*) &p,
              PROTO_OUTGOING_RESULT_SUCCESS);
          break;
        case COM_CONTENT_TOO_LONG:
          proto_new_packetOutgoingResult((proto_packetOutgoingResult*) &p,
              PROTO_OUTGOING_RESULT_CONTENT_TOO_LONG);
          break;
        case COM_CONNECTION_LOST:
          proto_new_packetOutgoingResult((proto_packetOutgoingResult*) &p,
              PROTO_OUTGOING_RESULT_CONNECTION_LOST);
          break;
        default:
          proto_new_packetOutgoingResult((proto_packetOutgoingResult*) &p,
              PROTO_OUTGOING_RESULT_INTERNAL_ERROR);
      }
      socket_push(req.ref, p);
      log_packet("com", &p, "sending");
    }

    // PJON reception
    com_message reception[SERVER_MAX_RECEPTION];
    n = com_receive(reception, SERVER_MAX_RECEPTION);
    for (unsigned int i = 0; i < n; i++) {
      proto_packet p;
      proto_new_packetIngoingMessage((proto_packetIngoingMessage*) &p,
          reception[i].src, reception[i].n, reception[i].data);
      socket_push(SOCKET_ALL, p);
    }
  }
}

/*
void write_slave_version(int sock)
{
  proto_packetVersion p = {PROTO_HEAD_VERSION, PROTO_VERSION};
  write(sock, &p, sizeof(p));
}

void write_slave_reception(int sock, const com_message *reception, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    com_message msg = reception[i];
    proto_packetIngoingMessage p;
    p.head = PROTO_HEAD_INGOING_MSG;
    p.src = msg.src;
    p.length = msg.n;
    memcpy(p.data, msg.data, msg.n);
    write(sock, &p, sizeof(p));
  }
}

void write_slave_results(const com_request *requests, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    com_request req = requests[i];
    proto_packetOutgoingResult p;
    p.head = PROTO_HEAD_OUTGOING_RESULT;
    p.result = req.state;
    write(req.ref, &p, sizeof(p));
  }
}
*/
