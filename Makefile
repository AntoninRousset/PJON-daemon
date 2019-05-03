# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <http://www.gnu.org/licenses/>.

.POSIX:

include config.mk

NAME = PJON-daemon

SRC = PJON-daemon.cpp communication.cpp logger.cpp
OBJ = $(SRC:.cpp=.o)

all: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(NAME) 

config.h:
	cp config.def.h config.h

.cpp.o:
	$(CC) $(CFLAGS) -c $<

PJON-daemon.o: config.h communication.hpp
communication.o: config.h

$(OBJ): config.h config.mk

clean:
	rm -f $(NAME) $(OBJ)

install: all 
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(NAME) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(NAME)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)

.PHONY: all clean dist install uninstall
