# NEEDED : export LD_LIBRARY_PATH="/usr/local/lib"

CC = gcc
LIBTOOL = libtool

TARGET = emirates_test

SRCS = emirates_test.c
OBJS = $(SRCS:.c=.o)
CFLAGS = -I. -I../include -I../.. -I../../jedex/include -std=gnu99 -D_GNU_SOURCE -Wall -g -fno-strict-aliasing -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = -L. -L/usr/local/lib -lemirates -lczmq -lsodium -ljedex -ljansson \
          -lzmq -lpthread -lrt

DEPENDS = .depends

.PHONY : all clean depend

all: depend $(TARGET)

$(TARGET): $(OBJS) 
	$(LIBTOOL) --mode=link $(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)

