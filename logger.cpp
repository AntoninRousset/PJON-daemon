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

#include "logger.hpp"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

static void log(const char *module, const char *prefix, const char *suffix,
    const char *format, va_list ap);
static bool print_iso_time(FILE *fo);
static int packet_to_str(const proto_packet *packet, char *str, size_t size);

static FILE ** outputs = nullptr;
static size_t outputs_n = 0;
static bool colors = true;
static char *time_format = nullptr;

void log_init(size_t n, FILE *fo[], bool c, const char *tf)
{
  if (outputs)
    free(outputs);
  if (time_format)
    free(time_format);

  outputs = (FILE**) malloc(n*sizeof(FILE*));
  memcpy(outputs, fo, n*sizeof(FILE*));
  outputs_n = n;
  colors = c;

  time_format = (char*) malloc((strlen(tf)+1)*sizeof(char));
  strcpy(time_format, tf);

  log_info("logger", "Initialization");
}

void log_info(const char *module, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log(module, "", nullptr, format, ap);
}

void log_warn(const char *module, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log(module, "WARNING : ", nullptr, format, ap);
}

void log_error(const char *module, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log(module, "ERROR : ", nullptr, format, ap);
}

void log_perror(const char *module, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log(module, "ERROR : ", strerror(errno), format, ap);
}

void log_packet(const char *module, const proto_packet *p,
    const char *format, ...)
{
  static char packet_str[LOG_MAX_PACKET_STR_LEN];
  va_list ap;
  va_start(ap, format);
  packet_to_str(p, packet_str, LOG_MAX_PACKET_STR_LEN);
  log(module, "PACKET : ", packet_str, format, ap);
}

void log(const char *module, const char *prefix, const char *suffix,
    const char *format, va_list ap)
{
  for (unsigned int i = 0; i < outputs_n; i++) {
    if (outputs[i]) {
      if (time_format)
        print_iso_time(outputs[i]);
      if (module)
        printf("%s - ", module);
      if (prefix)
      printf("%s", prefix);
      if (format)
        vprintf(format, ap);
      if (suffix)
        printf(" : %s", suffix);
      printf("\n");
    }
  }
}

bool print_iso_time(FILE *fo)
{
  char outstr[LOG_MAX_PACKET_STR_LEN];
  time_t t;
  struct tm *tmp;

  t = time(NULL);
  tmp = localtime(&t);
  if (tmp == NULL) {
    fprintf(fo, "Time fail: %s - ", strerror(errno));
    return false;
  }

  if (strftime(outstr, sizeof(outstr), time_format, tmp) == 0) {
    fprintf(fo, "Time fail: strftime returned 0 - ");
    return false;
  }

  fprintf(fo, "%s - ", outstr);
  return true;
}

int packet_to_str(const proto_packet *packet, char *str, size_t size)
{

  if (packet->head == PROTO_HEAD_VERSION) {
    auto *p = (proto_packetVersion*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_VERSION (0x%02x)\n"
        "\tversion: '%s'\n"
        "}", PROTO_HEAD_VERSION, p->version);
  }

  if (packet->head == PROTO_HEAD_INFO) {
    auto *p = (proto_packetInfo*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_INFO (0x%02x)\n"
        "\tcode: 0x%04x\n"
        "}", PROTO_HEAD_INFO, p->code);
  }

  if (packet->head == PROTO_HEAD_WARN) {
    auto *p = (proto_packetWarn*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_WARN (0x%02x)\n"
        "\tcode: 0x%04x\n"
        "}", PROTO_HEAD_WARN, p->code);
  }

  if (packet->head == PROTO_HEAD_ERROR) {
    auto *p = (proto_packetError*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_ERROR (0x%02x)\n"
        "\tcode: 0x%04x\n"
        "}", PROTO_HEAD_ERROR, p->code);
  }

  if (packet->head == PROTO_HEAD_INGOING_MSG) {
    auto *p = (proto_packetIngoingMessage*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_INGOING_MSG (0x%02x)\n"
        "\tsrc: 0x%02x\n"
        "\tlength: %d\n"
        "\tdata: ...\n"
        "}", PROTO_HEAD_INGOING_MSG, p->src, p->length);
  }

  if (packet->head == PROTO_HEAD_OUTGOING_MSG) {
    auto *p = (proto_packetOutgoingMessage*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_OUTGOING_MSG (0x%02x)\n"
        "\tdest: 0x%02x\n"
        "\tlength: %d\n"
        "\tdata: ...\n"
        "}", PROTO_HEAD_OUTGOING_MSG, p->dest, p->length);
  }

  if (packet->head == PROTO_HEAD_OUTGOING_RESULT) {
    auto *p = (proto_packetOutgoingResult*) packet;
    return snprintf(str, size, "{\n"
        "\thead: PROTO_HEAD_OUTGOING_RESULT (0x%02x)\n"
        "\tcode: 0x%04x\n"
        "}", PROTO_HEAD_OUTGOING_RESULT, p->result);
  }


  return 0;
}

