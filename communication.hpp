#pragma once

//TODO logger
//TODO remove using namespace

#include <stdint.h>
#include <string.h>

#define PJON_PACKET_MAX_LENGTH 50

typedef uint8_t PJON_id_t;
typedef int16_t Reference_t;

enum RequestState : int8_t {
  COM_PENDING = 0,
  COM_SUCCESS = 1,
  COM_CONTENT_TOO_LONG = -1, 
  COM_CONNECTION_LOST  = -2
};

typedef struct {
  Reference_t r;
  enum RequestState state;
} Request_t;

void com_init(PJON_id_t id, unsigned max_attempts=32, unsigned int initial_period_ms=1,
    unsigned int period_factor=2);

bool com_connect(const char * dev, uint32_t baudrate);

// return False in case of failure (e.g. stack is full)
bool com_push(Reference_t r, PJON_id_t dest, size_t n, const void* data);

void com_cancel(Reference_t r);

size_t com_send(Request_t * results, size_t n_max);

void com_quit();

