# Emirates Makefile

OPTIMIZATION?=-O0

CC=gcc
STD= -std=gnu99 -D_REENTRANT -D_GNU_SOURCE
WARN= -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -fno-strict-aliasing
OPT= $(OPTIMIZATION)


FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
FINAL_CFLAGS+= -I. -I../include -I../czmq/include -I../.. -I../../jedex/include -I../usr/local/include
LDFALGS=-L/usr/local/lib
FINAL_LDFLAGS= $(LDFLAGS) -g -rdynamic -ggdb $(ARCH)
FINAL_LIBS= -lczmq -lsodium -lzmq -lm -pthread $(ARCH)
DEBUG= -g -ggdb

EMIRATES_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

PREFIX?=/usr/local
INSTALL_LIB= $(PREFIX)/lib
INSTALL= cp -pf

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
LIBCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)$(CC)$(ENDCOLOR) $(SRCCOLOR)$<$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)link$(ENDCOLOR) $(LIBCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_INSTALL = @printf '    %b %b\n' $(LINKCOLOR)install$(ENDCOLOR) $(LIBCOLOR)$@$(ENDCOLOR) 1>&2;
endif

LIB_OBJS += emirates_main.o
LIB_OBJS += zcommon.o
LIB_OBJS += zpub.o
LIB_OBJS += zsub.o
LIB_OBJS += zrequester.o
LIB_OBJS += zresponder.o

ZREQUESTER_RESPONDER_OBJS += zrequester_responder.o
ZXPUB_SUB_OBJS += zxpub_xsub.o

LIB_FILE=libemirates.a
ZREQUESTER_RESPONDER = zrequester_responder
ZXPUB_SUB = zxpub_xsub

QUIET_AR= @echo '   ' ar $@;
RM = rm -f
AR = ar

OBJECTS += $(LIB_OBJS)
OBJECTS += $(ZREQUESTER_RESPONDER_OBJS)
OBJECTS += $(ZXPUB_SUB_OBJS)

dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))


$(dep_dirs):
	@mkdir -p $@

missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
dep_file = $(dir $@).depend/$(notdir $@).d
dep_args = -MF $(dep_file) -MQ $@ -MMD -MP

all: $(LIB_FILE) $(ZREQUESTER_RESPONDER) $(ZXPUB_SUB)
.PHONY : all

$(LIB_FILE): $(dep_dirs) $(LIB_OBJS)
	$(QUIET_AR)$(RM) $@ && $(AR) rcs $@ $(LIB_OBJS)

$(ZREQUESTER_RESPONDER): $(ZREQUESTER_RESPONDER_OBJS)
	$(EMIRATES_LD) -o $@ $^ $(FINAL_LIBS)

$(ZXPUB_SUB): $(ZXPUB_SUB_OBJS)
	$(EMIRATES_LD) -o $@ $^ $(FINAL_LIBS)

clean:
	rm -rf $(LIB_FILE) $(ZREQUESTER_RESPONDER) $(ZXPUB_SUB) *.o *.gcda *.gcov

distclean: clean
	-(rm -f $(INSTALL_LIB)/$(LIB_FILE) $(ZREQUESTER_RESPONDER) $(ZXPUB_SUB))

.PHONY: distclean

$(OBJECTS): %.o: %.c $(missing_dep_files)
	$(QUIET_CC)$(CC) -o $*.o -c $(dep_args) $(FINAL_CFLAGS) $<

install:
	$(QUIET_INSTALL)$(INSTALL) $(LIB_FILE) $(INSTALL_LIB)
