#include "communication.hpp"
#include <stdio.h> // for log

using namespace std;

Packet::Packet(PJON_id_t dest, size_t n, const void* data)
{
  this->dest = dest;
  this->attempts = 0;
  this->length = n;
  this->registration = PJON_MICROS();
  this->state = (n > PJON_PACKET_MAX_LENGTH) ? PJON_CONTENT_TOO_LONG : PJON_TO_BE_SENT;
  this->timing = this->registration;

  if (this->state != PJON_CONTENT_TOO_LONG)
    memcpy(this->content, data, n); 
}


Request::Request(Reference_t r, enum RequestState state)
{
  this->r = r;
  this->state = state;
}


Communication::Communication(PJON_id_t id, unsigned int max_attempts,
    unsigned int initial_period_ms, unsigned int period_factor)
  : bus(id), max_attempts(max_attempts), initial_period(initial_period_ms),
  period_factor(period_factor)
{
  this->log("** New communication initialiation **");
}


bool Communication::connect(const char* dev, uint32_t baudrate)
{
  // open serial
  this->log("\t* Opening serial device: %s", dev);
  this->bus.strategy.set_serial(serialOpen(dev, baudrate));
  if (bus.strategy.serial < 0) {
    this->log("\t\t-> Failed to open serial device: %s", dev);
    return false;
  }

  // setting bus
  this->log("\t* Setting up bus with baudrate = %ld", baudrate);
  bus.strategy.set_baud_rate(baudrate);
  bus.set_custom_pointer(this);
  bus.set_asynchronous_acknowledge(true);
  bus.begin();

  return true;
}


bool Communication::push(Reference_t r, PJON_id_t dest, size_t n, const void* data)
{
  Packet p = Packet(dest, n, data);
  p.period = this->initial_period;
  return this->packets.emplace(r, p).second;
}


void Communication::cancel(Reference_t r)
{
  this->packets.erase(r);
}


size_t Communication::send(Request *results, size_t n_max)
{
  size_t i = 0;
  for (auto &it : this->packets) {

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
      results[i] = Request(r, CONTENT_TOO_LONG);
      i++;
      this->cancel(r);
    }

    // try to send packet
    p.state = this->bus.send_packet(p.dest, (char*) p.content, p.length);
    p.attempts++;
    p.timing = PJON_MICROS();
    p.period *= this->period_factor;

    // SUCCESS 
    if (p.state == SUCCESS) {
      results[i] = Request(r, SUCCESS);
      i++;
      this->cancel(r);
    }

    // too much attempts -> CONNECTION_LOST
    else if (p.attempts > this->max_attempts) {
      results[i] = Request(r, SUCCESS);
      i++;
      this->cancel(r);
    }

  }

  return i;
}


void Communication::quit()
{
}


void Communication::log(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  printf("\n");
}

