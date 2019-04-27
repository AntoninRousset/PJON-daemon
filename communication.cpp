#include "communication.hpp"
#include <map>
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
    uint32_t registration;
    uint32_t timing;
    uint32_t period;
    uint16_t length;
    uint16_t state;
    PJON_id_t dest;
    uint8_t  attempts;
    char     content[PJON_PACKET_MAX_LENGTH];

};

static PJON<ThroughSerial> bus;
static std::map<Reference_t, Packet> packets;
static unsigned int max_attempts;
static unsigned int initial_period;
static unsigned int period_factor;
static void log(const char* format, ...);

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

void com_init(PJON_id_t id, unsigned int max_attempts, unsigned int initial_period_ms,
    unsigned int period_factor)
{
  bus.set_id(id);
  max_attempts = max_attempts;
  initial_period = initial_period_ms;
  period_factor = period_factor;
}

bool com_connect(const char* dev, uint32_t baudrate)
{
  log("** New communication initialiation **");

  // open serial
  log("\t* Opening serial device: %s", dev);
  bus.strategy.set_serial(serialOpen(dev, baudrate));
  if (bus.strategy.serial < 0) {
    log("\t\t-> Failed to open serial device: %s", dev);
    return false;
  }

  // setting bus
  log("\t* Setting up bus with baudrate = %ld", baudrate);
  bus.strategy.set_baud_rate(baudrate);
  bus.set_asynchronous_acknowledge(true);
  bus.begin();

  return true;
}

bool com_push(Reference_t r, PJON_id_t dest, size_t n, const void* data)
{
  Packet p = Packet(dest, n, data);
  p.period = initial_period;
  return packets.emplace(r, p).second;
}

void com_cancel(Reference_t r)
{
  packets.erase(r);
}

size_t com_send(Request_t * results, size_t n_max)
{
  size_t i = 0;
  for (auto &it : packets) {

    auto &r = it.first;
    auto &p = it.second;

    // break in case of full results array
    if (i >= n_max)
      return i;

    // send only if dt >= period
    if (PJON_MICROS() - p.timing < p.period)
      continue;

    // CONTENT_TOO_LONG
    if (p.state) {
      results[i] = (Request_t){r, COM_CONTENT_TOO_LONG};
      i++;
      com_cancel(r);
    }

    // try to send packet
    p.state = bus.send_packet(p.dest, (char*) p.content, p.length);
    p.attempts++;
    p.timing = PJON_MICROS();
    p.period *= period_factor;

    // SUCCESS 
    if (p.state == COM_SUCCESS) {
      results[i] = (Request_t){r, COM_SUCCESS};
      i++;
      com_cancel(r);
    }

    // too much attempts -> CONNECTION_LOST
    else if (p.attempts > max_attempts) {
      results[i] = (Request_t){r, COM_CONNECTION_LOST};
      i++;
      com_cancel(r);
    }

  }

  return i;
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

