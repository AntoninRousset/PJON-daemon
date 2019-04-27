#pragma once

#include <stdint.h>
#include <map>

#define TS_RESPONSE_TIME_OUT 35000
/* Maximum accepted timeframe between transmission and synchronous
   acknowledgement. This timeframe is affected by latency and CRC computation.
   Could be necessary to higher this value if devices are separated by long
   physical distance and or if transmitting long packets. */

#define PJON_INCLUDE_TS true // Include only ThroughSerial
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <PJON.h>

//TODO logger

using namespace std;

typedef uint8_t PJON_id_t;
typedef int16_t Reference_t;

enum RequestState : int8_t {
  PENDING = 0,
  SUCCESS = 1,
  CONTENT_TOO_LONG = -1, 
  CONNECTION_LOST  = -2
};

class Request {
 
  public:

    Request(Reference_t r, enum RequestState state);
    Reference_t r;
    enum RequestState state;

};

class Packet : public PJON_Packet {

  public:

    Packet(PJON_id_t dest, size_t n, const void* data);
    PJON_id_t dest;
    unsigned int period;

};


class Communication {

  public:

    Communication(PJON_id_t id, unsigned int max_attempts = 32,
        unsigned int initial_period_ms = 1, unsigned int period_factor = 2);

    bool connect(const char* dev, uint32_t baudrate);

    // return False in case of failure (e.g. stack is full)
    bool push(Reference_t r, PJON_id_t dest, size_t n, const void* data);

    void cancel(Reference_t r);

    size_t send(Request *results, size_t n_max);

    void quit();

  private:
    PJON<ThroughSerial> bus;
    map<Reference_t, Packet> packets;
    PJON_id_t id;
    unsigned int max_attempts;
    unsigned int initial_period;
    unsigned int period_factor;


    static void error_handler(uint8_t code, uint16_t data, void* custom_pointer);
    static void reception_handler(uint8_t *data, uint16_t n, const PJON_Packet_Info &info);
    void log(const char* format, ...);
};

