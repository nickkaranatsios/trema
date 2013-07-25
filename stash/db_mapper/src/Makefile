# Makefile for test programs

OPTIMIZATION?=-O0

CC=gcc
STD= -std=gnu99 -D_REENTRANT -D_GNU_SOURCE
WARN= -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -fno-strict-aliasing
OPT= $(OPTIMIZATION)


FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
FINAL_CFLAGS += -I. -I../src -I../include -I../czmq/include -I../..
FINAL_CFLAGS += -I../../jedex/include
FINAL_CFLAGS += -I ../../emirates/include
FINAL_CFLAGS += -I ../../emirates/src
FINAL_CFLAGS += -I../../../network_controller/trema/objects/openflow -I../../../network_controller/trema/src/lib -I/usr/local/include -I/usr/include/mysql
FINAL_LDFLAGS= $(LDFLAGS) -L/usr/lib/mysql -g -rdynamic -ggdb $(ARCH)
FINAL_LIBS= -ltrema -lsqlite3 -lczmq -lsodium -ljedex -ljansson -lzmq -lmysqlclient -lm -pthread $(ARCH)
DEBUG= -g -ggdb

DB_MAPPER_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

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

DB_MAPPER=db_mapper
DB_MAPPER_OBJS += db_mapper.o
DB_MAPPER_OBJS += parse_options.o
DB_MAPPER_OBJS += wrapper.o

RM = rm -f

all: $(DB_MAPPER)

.PHONY: all

OBJECTS += $(DB_MAPPER_OBJS)

dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))


$(dep_dirs):
	@mkdir -p $@

missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
dep_file = $(dir $@).depend/$(notdir $@).d
dep_args = -MF $(dep_file) -MQ $@ -MMD -MP

$(DB_MAPPER): $(DB_MAPPER_OBJS)
	$(DB_MAPPER_LD) -o $@ $^ $(FINAL_LIBS)

clean:
	rm -rf $(DB_MAPPER) *.o *.gcda *.gcov

.PHONY: clean

distclean: clean
	-(rm -f $(DB_MAPPER))

.PHONY: distclean

$(OBJECTS): %.o: %.c $(dep_dirs) $(missing_dep_files)
	$(QUIET_CC)$(CC) -o $*.o -c $(dep_args) $(FINAL_CFLAGS) $<
