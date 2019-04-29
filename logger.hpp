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

#include <stdio.h>

// set the n log output files fo and the format tf
// n: number of outputs (set to 0 for no output)
// fo: list of output file (don't hesitate to use stdout or stderr)
// c: enable colors
// tf: time format as specified for strftime (set to nullptr for the time not
// to be written)
void log_init(size_t n, FILE *fo[], bool c=true, const char *tf="%FT%TZ");

// log, used as printf withou \n at the end
void log_info(const char *format, ...);
void log_warn(const char *format, ...);
void log_error(const char *format, ...);
