#
# Copyright (C) 2011 NEC Corporation
# NEC Confidential
#
# Makefile for carrier-edge system common library (cesc)

TREMA = ../network_controller/trema

CC = gcc
AR = ar
RANLIB = ranlib

CFLAGS += $(shell $(TREMA)/trema-config --cflags) 
CFLAGS += -std=gnu99 -DCARRIER_EDGE -D_GNU_SOURCE -Wall -g -fno-strict-aliasing -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -O0

TARGET = libcesc.a
SRCS = daemon.c
SRCS += doubly_linked_list.c
SRCS += hash_table.c
SRCS += config.c
SRCS += cache.c
SRCS += strbuf.c

OBJS = $(SRCS:.c=.o)
OBJS += $(TREMA)/objects/lib/wrapper.o
OBJS += $(TREMA)/objects/lib/trema_wrapper.o

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

$(TARGET): $(OBJS)
	$(AR) -cq $@ $(OBJS)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend: 
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)

