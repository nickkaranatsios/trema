# Makefile for test programs

OPTIMIZATION?=-O2

CC=gcc
STD= -std=gnu99 -D_REENTRANT -D_GNU_SOURCE
WARN= -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -fno-strict-aliasing
OPT= $(OPTIMIZATION)


FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
FINAL_CFLAGS+= -I. -I../src -I../include -I../czmq/include -I../.. -I../../jedex/include -I../../../../trema/objects/openflow -I../../../../trema/src/lib -I../usr/local/include
FINAL_LDFLAGS= $(LDFLAGS) -g -rdynamic -ggdb $(ARCH)
FINAL_LIBS= -ltrema -lsqlite3 -lczmq -lsodium -ljedex -ljansson -lzmq -lm -pthread $(ARCH)
DEBUG= -g -ggdb

EMIRATES_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
LIBCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)$(CC)$(ENDCOLOR) $(SRCCOLOR)$<$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)link$(ENDCOLOR) $(LIBCOLOR)$@$(ENDCOLOR) 1>&2;
endif

SUBSCRIBER_ONLY=subscriber_only
SUBSCRIBER_ONLY_OBJS := subscriber_only.o
PUBLISHER_ONLY=publisher_only
PUBLISHER_ONLY_OBJS := publisher_only.o
REQUESTER_ONLY := requester_only
REQUESTER_ONLY_OBJS := requester_only.o
RESPONDER_ONLY := responder_only
RESPONDER_ONLY_OBJS := responder_only.o

REQUESTER_RESPONDER_TEST=requester_responder_test
REQUESTER_RESPONDER_TEST_OBJS := requester_responder_test.o


RM = rm -f

all: $(SUBSCRIBER_ONLY) \
	$(PUBLISHER_ONLY) \
	$(REQUESTER_ONLY) \
  $(RESPONDER_ONLY)

.PHONY: all

OBJECTS += $(SUBSCRIBER_ONLY_OBJS)
OBJECTS += $(PUBLISHER_ONLY_OBJS)
OBJECTS += $(REQUESTER_ONLY_OBJS)
OBJECTS += $(RESPONDER_ONLY_OBJS)
OBJECTS += $(REQUESTER_RESPONDER_TEST_OBJS)

dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))


$(dep_dirs):
	@mkdir -p $@

missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
dep_file = $(dir $@).depend/$(notdir $@).d
dep_args = -MF $(dep_file) -MQ $@ -MMD -MP

$(SUBSCRIBER_ONLY): $(SUBSCRIBER_ONLY_OBJS)
	$(EMIRATES_LD) -o $@ $^ ../src/libemirates.a $(FINAL_LIBS)

$(PUBLISHER_ONLY): $(PUBLISHER_ONLY_OBJS)
	$(EMIRATES_LD) -o $@ $^ ../src/libemirates.a $(FINAL_LIBS)

$(REQUESTER_ONLY): $(REQUESTER_ONLY_OBJS)
	$(EMIRATES_LD) -o $@ $^ ../src/libemirates.a $(FINAL_LIBS)

$(RESPONDER_ONLY): $(RESPONDER_ONLY_OBJS)
	$(EMIRATES_LD) -o $@ $^ ../src/libemirates.a $(FINAL_LIBS)

$(REQUESTER_RESPONDER_TEST): $(REQUESTER_RESPONDER_TEST_OBJS)
	$(EMIRATES_LD) -o $@ $^ ../src/libemirates.a $(FINAL_LIBS)

clean:
	rm -rf $(SUBSCRIBER_ONLY) \
    $(PUBLISHER_ONLY) \
    $(REQUESTER_ONLY) \
    $(RESPONDER_ONLY) \
    $(REQUESTER_RESPONDER_TEST) \
    *.o *.gcda *.gcov

.PHONY: clean

distclean: clean
	-(rm -f $(SUBSCRIBER_ONLY))

.PHONY: distclean

$(OBJECTS): %.o: %.c $(dep_dirs) $(missing_dep_files)
	$(QUIET_CC)$(CC) -o $*.o -c $(dep_args) $(FINAL_CFLAGS) $<
