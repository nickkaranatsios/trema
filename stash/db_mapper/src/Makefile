# Makefile for db_mapper program
TREMA = ../../../network_controller/trema
CC = gcc
CC = gcc
AR = ar
RANLIB = ranlib
APILIB = -lczmq -lsodium -lzmq -lmysqlclient -lm -pthread
APIPATH = -L. -L/usr/local/lib -L/usr/lib/mysql -L/usr/lib

CFLAGS += -I../include
CFLAGS += -I../.. 
CFLAGS += -I../../jedex/include 
CFLAGS += -I../../emirates/include 
CFLAGS += $(shell $(TREMA)/trema-config --cflags) 
CFLAGS += -I/usr/include/mysql
CFLAGS += -I/usr/local/include
CFLAGS += -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -O0

LDFLAGS += $(shell $(TREMA)/trema-config --libs)

TARGET_DB_MAPPER = db_mapper
SRCS_DB_MAPPER += ../../../network_controller/trema/src/lib/wrapper.c
SRCS_DB_MAPPER += ../../../network_controller/trema/src/lib/trema_wrapper.c
SRCS_DB_MAPPER += db_mapper.c
SRCS_DB_MAPPER += wrapper.c
SRCS_DB_MAPPER += parse_options.c
SRCS_DB_MAPPER += strbuf.c
SRCS_DB_MAPPER += config.c


TARGETS = $(TARGET_DB_MAPPER)
SRCS = $(SRCS_DB_MAPPER)
OBJS_DB_MAPPER = $(SRCS_DB_MAPPER:.c=.o)

DEPENDS = .depends

.PHONY: all depend clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGET_DB_MAPPER): $(OBJS_DB_MAPPER)
	$(CC) $(OBJS_DB_MAPPER) $(LDFLAGS) $(APILIB) $(APIPATH) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
