# Jedex Makefile

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

OPTIMIZATION?=-O2

#gcc -c  -I../include -I../..  -g -std=gnu99 -D_REENTRANT -D_GNU_SOURCE -Wall -Wno-unused -fno-strict-aliasing  -DBASE_THREADSAFE -I. st.c
STD= -std=gnu99 -D_REENTRANT -D_GNU_SOURCE
WARN= -Wall -W -Wstrict-prototypes -Wno-unused -fno-strict-aliasing -Wwrite-strings
OPT= $(OPTIMIZATION)


FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
FINAL_CFLAGS+= -I../include -I../.. -I../usr/local/include
FINAL_LDFLAGS= $(LDFLAGS) -g -rdynamic -ggdb $(ARCH)
FINAL_LIBS= -lm -pthread $(ARCH)
DEBUG= -g -ggdb
JEDEX_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
JEDEX_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)
JEDEX_INSTALL=$(QUIET_INSTALL)$(INSTALL)

PREFIX?=/usr/local
INSTALL_LIB= $(PREFIX)/lib
INSTALL= cp -pf

ifeq ($(uname_S),Darwin)
endif

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
LIBCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(LIBCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_INSTALL = @printf '    %b %b\n' $(LINKCOLOR)INSTALL$(ENDCOLOR) $(LIBCOLOR)$@$(ENDCOLOR) 1>&2;
endif

LIB_OBJS += init.o
LIB_OBJS += allocation.o
LIB_OBJS += linked_list_safe.o
LIB_OBJS += generic.o
LIB_OBJS += generic_array.o
LIB_OBJS += generic_boolean.o
LIB_OBJS += generic_bytes.o
LIB_OBJS += generic_double.o
LIB_OBJS += generic_float.o
LIB_OBJS += generic_int.o
LIB_OBJS += generic_link.o
LIB_OBJS += generic_long.o
LIB_OBJS += generic_map.o
LIB_OBJS += generic_null.o
LIB_OBJS += generic_record.o
LIB_OBJS += generic_string.o
LIB_OBJS += generic_union.o
LIB_OBJS += jedex_schema.o
LIB_OBJS += memoize.o
LIB_OBJS += raw_string.o
LIB_OBJS += raw_array.o
LIB_OBJS += raw_map.o
LIB_OBJS += st.o
LIB_OBJS += value_json.o
LIB_OBJS += wrapped_buffer.o

LIB_FILE=libjedex.a

QUIET_AR= @echo '   ' AR $@;
RM = rm -f
AR = ar

OBJECTS := $(LIB_OBJS)
dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)

$(LIB_FILE):$(LIB_OBJS)
	$(QUIET_AR)$(RM) $@ && $(AR) rcs $@ $(LIB_OBJS)

# Prerequisites target
.make-prerequisites:
	@touch $@


persist-settings: distclean
  echo STD=$(STD) >> .make-settings
  echo WARN=$(WARN) >> .make-settings
  echo OPT=$(OPT) >> .make-settings
  echo CFLAGS=$(CFLAGS) >> .make-settings
  echo LDFLAGS=$(LDFLAGS) >> .make-settings
  echo PREV_FINAL_CFLAGS=$(FINAL_CFLAGS) >> .make-settings
  echo PREV_FINAL_LDFLAGS=$(FINAL_LDFLAGS) >> .make-settings

.PHONY: persist-settings

# Clean everything, persist settings and build dependencies if anything changed
ifneq ($(strip $(PREV_FINAL_CFLAGS)), $(strip $(FINAL_CFLAGS)))
.make-prerequisites: persist-settings
endif

ifneq ($(strip $(PREV_FINAL_LDFLAGS)), $(strip $(FINAL_LDFLAGS)))
.make-prerequisites: persist-settings
endif

clean:
	rm -rf LIB_FILE *.o *.gcda *.gcov

distclean: clean
	-(rm -f $(INSTALL_LIB)/LIB_FILE )

.PHONY: distclean

%.o: %.c .make-prerequisites
	$(JEDEX_CC) -c $<
