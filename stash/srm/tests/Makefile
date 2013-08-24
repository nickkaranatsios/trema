# Makefile for test programs
TREMA = ../../network_controller/trema

CC = gcc

APILIB = -lczmq -lsodium -lzmq -ljansson -lhiredis -lm -lpthread
APIPATH = -L. -L../../lib -L../../lib/emirates/src -L../../lib/jedex/src -L/usr/local/lib -L/usr/lib

CFLAGS += -I../../lib/emirates/include
CFLAGS += -I../../lib
CFLAGS += -I../../lib/jedex/include
CFLAGS += $(shell $(TREMA)/trema-config --cflags)
CFLAGS += -I/usr/include/mysql
CFLAGS += -I/usr/local/include
CFLAGS += -std=gnu99 -D_GNU_SOURCE -DHAVE_CARRIER_EDGE -g -fno-strict-aliasing -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -O0

LDFLAGS += -lemirates
LDFLAGS += -ljedex
LDFLAGS += -lcesc
LDFLAGS += $(APILIB)
LDFLAGS += $(shell $(TREMA)/trema-config --libs)

TARGET_RESPONDER = responder
SRCS_RESPONDER = responder.c

TARGETS = $(TARGET_RESPONDER)

SRCS = $(SRCS_RESPONDER)
OBJS_RESPONDER = $(SRCS_RESPONDER:.c=.o)

OBJS = $(OBJS_RESPONDER)

DEPENDS = .depends

.PHONY: all depend clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGET_RESPONDER): $(OBJS_RESPONDER)
	$(CC) $(OBJS_RESPONDER) $(APIPATH) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
