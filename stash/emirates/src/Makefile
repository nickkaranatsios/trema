CC = gcc
CC = gcc
AR = ar
RANLIB = ranlib
APILIB = -lczmq -lsodium -lzmq -lm -pthread
APIPATH = -L. -L/usr/local/lib -L/usr/lib

CFLAGS = -I../include -I../czmq/include -I../.. -I../../jedex/include -I../usr/local/include
CFLAGS += -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -O0

TARGET_LIB = libemirates.a
SRCS_LIBS = emirates_main.c zcommon.c zpub.c zsub.c zrequester.c zresponder.c

TARGET_RR = zrequester_responder
SRCS_RR = zrequester_responder.c

TARGET_PS = zxpub_xsub
SRCS_PS = zxpub_xsub.c
OBJS_LIB = $(SRCS_LIB:.c=.o)

TARGETS = $(TARGET_LIB) $(TARGET_RR) $(TARGET_PS)
SRCS = $(SRCS_LIBS) $(SRCS_RR) $(SRCS_PS)
OBJS_RR = $(SRCS_RR:.c=.o)
OBJS_PS = $(SRCS_PS:.c=.o)

DEPENDS = .depends

.PHONY: all depend clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(TARGET_RR): $(OBJS_RR)
	$(CC) $(OBJS_RR) $(LDFLAGS) $(APILIB) $(APIPATH) -o $@

$(TARGET_PS): $(OBJS_PS)
	$(CC) $(OBJS_PS) $(LDFLAGS) $(APILIB) $(APIPATH) -o $@

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) -cq $@ $(OBJS_LIB)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
