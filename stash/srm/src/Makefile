#
# Copyright (C) 2011 NEC Corporation
#

LIB = ../../lib/src
CC = gcc

TARGET = system_resource_manager

SRCS = system_resource_manager.c srm_dmy_core.c srm_data_pool.c \
       ob_transmission.c sc_transmission.c nc_transmission.c \
       create_json_file.c read_properties.c send_json_file.c 
OBJS = $(SRCS:.c=.o)

CFLAGS = -std=gnu99 -D_GNU_SOURCE -g -Wall -I. -I$(LIB)
LDFLAGS = -L. -L/usr/local/lib -lipc -lmqx -lzmq -lpthread $(LIB)/daemon.a $(LIB)/linked_list.a

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)
