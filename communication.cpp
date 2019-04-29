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
#include <map>
#include <vector>
#include <memory>
#include <stdio.h> // for log

/* Maximum accepted timeframe between transmission and synchronous
   acknowledgement. This timeframe is affected by latency and CRC computation.
   Could be necessary to higher this value if devices are separated by long
   physical distance and or if transmitting long packets. */
#define TS_RESPONSE_TIME_OUT 35000
#define PJON_INCLUDE_TS true // Only include ThroughSerial
#include <stdlib.h>
#include <unistd.h>
#include <PJON.h>

class Packet {

  public:

    Packet(PJON_id_t dest, size_t n, const void* data);
    float period;
    uint32_t registration;
    uint32_t timing;
    uint16_t length;
    uint16_t state;
    PJON_id_t dest;
    uint8_t  attempts;
    char     content[PJON_PACKET_MAX_LENGTH];

};

static PJON<ThroughSerial> bus;
static std::map<Reference_t, Packet> packets;
static unsigned int max_attempts;
static float initial_period;
static float period_factor;
static Message_t reception[COM_MAX_INCOMING_MESSAGES];
static unsigned int reception_p = 0;
static void log(const char* format, ...);
static void receiver(uint8_t * data, uint16_t n, const PJON_Packet_Info &packet_info);

Packet::Packet(PJON_id_t dest, size_t n, const void* data)
{
  this->registration = PJON_MICROS();
  this->timing = this->registration;
  this->period = initial_period;
  this->length = n;
  this->state = (n > PJON_PACKET_MAX_LENGTH) ? PJON_CONTENT_TOO_LONG : PJON_TO_BE_SENT;
  this->dest = dest;
  this->attempts = 0;
  if (this->state != PJON_CONTENT_TOO_LONG)
    memcpy(this->content, data, n); 
}

void com_init(PJON_id_t id, unsigned int ma, float ip, float pf)
{
  bus.set_id(id);
  bus.set_receiver(receiver);
  max_attempts = ma;
  initial_period = ip;
  period_factor = pf;
}

bool com_connect(const char* dev, uint32_t baudrate)
{
  log("** New communication initialiation **");

  // open serial
  log("\t* Opening serial device: %s", dev);
  bus.strategy.set_serial(serialOpen(dev, baudrate));
  if (!com_is_connected()) {
    log("\t\t-> Failed to open serial device: %s", dev);
    return false;
  }

  // setting bus
  log("\t* Setting up bus with baudrate = %ld", baudrate);
  bus.strategy.set_baud_rate(baudrate);
	bus.set_synchronous_acknowledge(true);
  bus.begin();

  return true;
}

bool com_is_connected(void)
{
  return bus.strategy.serial >= 0;
}

bool com_push(Reference_t r, PJON_id_t dest, size_t n, const void* data)
{
  packets.insert(std::pair<Reference_t, Packet>(r, Packet(dest, n, data)));
  return true;
}

void com_cancel(Reference_t r)
{
  printf("Cancel before %d\n", packets.size());
  auto it = packets.find(r);
  if (it != packets.end())
    packets.erase(r);
  printf("Cancel after %d\n", packets.size());
}

size_t com_send(Request_t * results, size_t n_max)
{
  size_t n = 0;
  auto to_cancel = std::vector<Reference_t>();

  for (auto &it : packets) {

    auto r = it.first;
    auto &p = it.second;

    // break in case of full results array
    if (n >= n_max)
      return n;

    // send only if dt >= period
    if (PJON_MICROS() - p.timing < p.period)
      continue;

    // CONTENT_TOO_LONG
    if (p.state == PJON_CONTENT_TOO_LONG) {
      printf("COM_CONTENT_TOO_LONG (%d)\n", p.state);
      results[n] = (Request_t){r, COM_CONTENT_TOO_LONG};
      n++;
      to_cancel.push_back(r);
      continue;
    }

    // try to send packet
    p.state = bus.send_packet(p.dest, (char*) p.content, p.length);
    printf("Try to send to 0x%02x (%d) -> %d\n", p.dest, p.timing, p.state);
    p.attempts++;
    p.timing = PJON_MICROS();
    p.period *= period_factor;

    // SUCCESS 
    if (p.state == PJON_ACK) {
      printf("COM_SUCCESS\n");
      results[n] = (Request_t){r, COM_SUCCESS};
      n++;
      to_cancel.push_back(r);
      continue;
    }

    // too much attempts -> CONNECTION_LOST
    if (p.attempts > max_attempts) {
      printf("COM_CONNECTION_LOST: %d/%d\n", p.attempts, max_attempts);
      results[n] = (Request_t){r, COM_CONNECTION_LOST};
      n++;
      to_cancel.push_back(r);
      continue;
    }

  }

  for (auto &r : to_cancel)
    com_cancel(r);

  return n;
}

size_t com_receive(Message_t * income, size_t n_max)
{
  bus.receive(1000);
}

void com_quit()
{
}

void log(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  printf("\n");
}

void receiver(uint8_t * data, uint16_t n, const PJON_Packet_Info &packet_info)
{
  reception[reception_p] = (Message_t) {packet_info.sender_id, n};
  memcpy(&reception[reception_p].data, data, sizeof(reception[reception_p].data));
  reception_p++;
}
