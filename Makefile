LIBS= -lstdc++ -lgfortran -lcrypt -lm -lrt
CC = gcc
LDIR = PJON/src
CFLAGS = -g -Wall -Wextra -DLINUX -I. -I$(LDIR) -std=gnu++17

.PHONY: default all clean

default: all
all: server client

SERVER_DEPS = server.cpp communication.cpp logger.cpp
CLIENT_DEPS = client.cpp logger.cpp

SERVER_OBJECTS = $(patsubst %.cpp, %.o, $(SERVER_DEPS))
CLIENT_OBJECTS = $(patsubst %.cpp, %.o, $(CLIENT_DEPS)) 
HEADERS = $(wildcard *.h)
HEADERS += $(wildcard *.hpp)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@

.PRECIOUS: server client $(SERVER_OBJECTS) $(CLIENT_OBJECTS)

server: $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) $(LIBS) -o $@

client: $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f client
	-rm -f server 

