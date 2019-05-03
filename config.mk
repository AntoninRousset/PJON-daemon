PREFIX = /usr/local

CC = gcc

INCS = -IPJON/src
LIBS = -lstdc++ -lgfortran -lcrypt -lm -lrt

CFLAGS = -g -Wall -Wextra -DLINUX $(INCS) -std=gnu++17
LDFLAGS = $(LIBS)

