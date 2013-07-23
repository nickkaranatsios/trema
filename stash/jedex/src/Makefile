# Jedex Makefile

OPTIMIZATION?=-O0

CC=gcc
STD= -std=gnu99 -D_REENTRANT -D_GNU_SOURCE
WARN= -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -fno-strict-aliasing
OPT= $(OPTIMIZATION)


FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
FINAL_CFLAGS+= -I../include -I../.. -I../usr/local/include
FINAL_LDFLAGS= $(LDFLAGS) -g -rdynamic -ggdb $(ARCH)
FINAL_LIBS= -lm -pthread $(ARCH)
DEBUG= -g -ggdb

JEDEX_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

PREFIX?=/usr/local
INSTALL_LIB= $(PREFIX)/lib
INSTALL= install -p

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

QUIET_AR= @echo '   ' ar $@;
RM = rm -f
AR = ar

OBJECTS := $(LIB_OBJS)

dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))


$(dep_dirs):
	@mkdir -p $@

missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
dep_file = $(dir $@).depend/$(notdir $@).d
dep_args = -MF $(dep_file) -MQ $@ -MMD -MP

all: $(LIB_FILE)
.PHONY : all

$(LIB_FILE): $(dep_dirs) $(LIB_OBJS)
	$(QUIET_AR)$(RM) $@ && $(AR) rcs $@ $(LIB_OBJS)

clean:
	rm -rf $(LIB_FILE) *.o *.gcda *.gcov

distclean: clean
	-(rm -f $(INSTALL_LIB)/$(LIB_FILE))

.PHONY: distclean

$(OBJECTS): %.o: %.c $(missing_dep_files)
	$(QUIET_CC)$(CC) -o $*.o -c $(dep_args) $(FINAL_CFLAGS) $<

install:
	$(QUIET_INSTALL)$(INSTALL) $(LIB_FILE) $(INSTALL_LIB)
