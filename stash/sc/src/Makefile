#
# Copyright (C) 2013 NEC Corporation
#

TREMA = ../../../network_controller/trema
CC = gcc
LIB = ../../../lib
LIB_SC = ../../lib

APILIB = -lczmq -lsodium -lzmq -ljansson -lmysqlclient -lhiredis -lsqlite3 -lvirt -lm -lpthread
APIPATH = -L$(LIB) -L$(LIB_SC) -L$(LIB)/emirates/src -L$(LIB)/jedex/src -L../.. -L/usr/local/lib -L/usr/lib

APILIB = -lczmq -lsodium -lzmq -ljansson -lmysqlclient -lhiredis -lsqlite3 -lvirt -lm -lpthread

CFLAGS += -I../../../lib
CFLAGS += $(shell $(TREMA)/trema-config --cflags)
CFLAGS += -I../../../lib/jedex/include
CFLAGS += -I../../../lib/emirates/include
CFLAGS += -I../../lib
CFLAGS += -std=gnu99 -D_GNU_SOURCE -DCARRIER_EDGE -g -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith

LDFLAGS += -lemirates
LDFLAGS += -ljedex
LDFLAGS += -lcesc
LDFLAGS += -lpmcontrol
LDFLAGS += -lvmcontrol
LDFLAGS += -lsmcontrol
LDFLAGS += -lssh2
LDFLAGS += $(APILIB)


TARGETS = sc_resource_manager
SRCS = sc_resource_manager.c
SRCS += parse_options.c
SRCS += init.c
SRCS += message.c
OBJS = $(SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all depend clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGETS): $(OBJS)
	$(CC) $(OBJS) $(APIPATH) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
