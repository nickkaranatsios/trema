#
# Copyright (C) 2011 NEC Corporation
#

TREMA = ../../network_controller/trema
CC = gcc

APILIB = -lczmq -lsodium -lzmq -ljansson -lmysqlclient -lhiredis -lsqlite3 -lm -lpthread
APIPATH = -L../../lib -L../../lib/emirates/src -L../../lib/jedex/src -L../.. -L/usr/local/lib -L/usr/lib

TARGET = system_resource_manager

SRCS = init.c
SRCS += parse_options.c
SRCS += message.c
SRCS += system_resource_manager.c

OBJS = $(SRCS:.c=.o)
OBJS += $(TREMA)/objects/lib/wrapper.o
OBJS += $(TREMA)/objects/lib/linked_list.o

CFLAGS += -I../../lib 
CFLAGS += -I../../lib/jedex/include 
CFLAGS += -I../../lib/emirates/include 
CFLAGS += $(shell $(TREMA)/trema-config --cflags) 
CFLAGS += -I/usr/include/mysql
CFLAGS += -I/usr/local/include
CFLAGS += -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -O0

LDFLAGS += -lemirates
LDFLAGS += -ljedex
LDFLAGS += -lcesc
LDFLAGS += $(APILIB)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(APIPATH) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)

