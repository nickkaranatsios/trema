# Makefile for jedex test programs
CC = gcc

APILIB = -ljansson -lm -lpthread
APIPATH = -L. -L/usr/local/lib -L/usr/lib

CFLAGS += -I ../include
CFLAGS += -I ../src
CFLAGS += -I ../..
CFLAGS += -I../usr/local/include
CFLAGS += -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -O0

LDFLAGS += -ljedex
LDFLAGS += $(APILIB)

TARGET_JEDEX_TEST = jedex_test
SRCS_JEDEX_TEST = jedex_test.c

TARGETS = $(TARGET_JEDEX_TEST)

SRCS = $(SRCS_JEDEX_TEST)
OBJS_JEDEX_TEST = $(SRCS_JEDEX_TEST:.c=.o)
OBJS = $(OBJS_JEDEX_TEST)

DEPENDS = .depends

.PHONY: all depend clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGET_JEDEX_TEST): $(OBJS_JEDEX_TEST)
	$(CC) $(OBJS_JEDEX_TEST) $(APIPATH) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
