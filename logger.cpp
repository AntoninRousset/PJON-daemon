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

static void log(const char* prefix, const char *format, va_list ap);
static bool print_iso_time(FILE *fo);

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
}

void log_info(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log("", format, ap);
}

void log_warn(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log("WARNING : ", format, ap);
}

void log_error(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  log("ERROR : ", format, ap);
}

void log(const char* prefix, const char *format, va_list ap)
{
  for (unsigned int i = 0; i < outputs_n; i++) {
    if (outputs[i]) {
      if (time_format)
        print_iso_time(outputs[i]);
      printf("%s", prefix);
      vprintf(format, ap);
      printf("\n");
    }
  }
}

bool print_iso_time(FILE *fo)
{
  char outstr[200];
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

