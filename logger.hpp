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

#pragma once

//TODO output colors

#include "protocol.hpp"
#include <stdio.h>

#ifndef LOG_MAX_PACKET_STR_LEN
#define LOG_MAX_PACKET_STR_LEN 256
#endif

// set the n log output files fo and the format tf
// n: number of outputs (set to 0 for no output)
// fo: list of output file (don't hesitate to use stdout or stderr)
// c: enable colors
// tf: time format as specified for strftime (set to nullptr for the time not
// to be written)
void log_init(size_t n, FILE *fo[], bool c=true, const char *tf="%FT%TZ");

// set log level
// 0: everything is printed
// 1: warnings and errors
// 3: only errors
void log_set_level(unsigned int l);

// log, used as printf withou \n at the end
void log_info(const char *module, const char *format, ...);
void log_warn(const char *module, const char *format, ...);
void log_error(const char *module, const char *format, ...);
void log_perror(const char *module, const char *format, ...);
void log_packet(const char *module, const proto_packet *p,
    const char *format, ...);
