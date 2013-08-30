#
# Copyright (C) 2011 NEC Corporation
# NEC Confidential
#

TREMA = ../../network_controller/trema
CC = gcc
AR = ar
RANLIB = ranlib
LIB = ../../lib

APILIB = -lssh2
APIPATH = -L$(LIB) -L/usr/local/lib

CFLAGS += $(shell $(TREMA)/trema-config --cflags)
CFLAGS += -I$(LIB)
CFLAGS += -I/usr/include/mysql
CFLAGS += -I/usr/local/include 
CFLAGS += -std=gnu99 -D_GNU_SOURCE -DCARRIER_EDGE -g -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith


LDFLAGS += $(APIPATH)
LDFLAGS += $(APILIB)
LDFLAGS += -lcesc

PMCONTROL = libpmcontrol.a

PMCONTROL_SRCS = snmp.c

PMCONTROL_OBJS = $(PMCONTROL_SRCS:.c=.o)
PMCONTROL_OBJS += $(TREMA)/objects/lib/linked_list.o
PMCONTROL_OBJS += $(TREMA)/objects/lib/utility.o
PMCONTROL_OBJS += $(TREMA)/objects/lib/wrapper.o
PMCONTROL_OBJS += $(TREMA)/objects/lib/trema_wrapper.o


VMCONTROL = libvmcontrol.a

VMCONTROL_SRCS = snmp.c
VMCONTROL_SRCS += vmm.c
VMCONTROL_SRCS += ssh.c

VMCONTROL_OBJS = $(VMCONTROL_SRCS:.c=.o)
VMCONTROL_OBJS += $(TREMA)/objects/lib/linked_list.o
PMCONTROL_OBJS += $(TREMA)/objects/lib/wrapper.o
PMCONTROL_OBJS += $(TREMA)/objects/lib/trema_wrapper.o

SMCONTROL = libsmcontrol.a

SMCONTROL_SRCS = snmp.c
SMCONTROL_SRCS += service_module.c

SMCONTROL_OBJS = $(SMCONTROL_SRCS:.c=.o)

TARGETS = $(PMCONTROL) $(VMCONTROL) $(SMCONTROL)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGETS)

$(PMCONTROL): $(PMCONTROL_OBJS)
	$(AR) -cq $@ $(PMCONTROL_OBJS) 
	$(RANLIB) $@

$(VMCONTROL): $(VMCONTROL_OBJS)
	$(AR) -cq $@ $(VMCONTROL_OBJS)
	$(RANLIB) $@

$(SMCONTROL): $(SMCONTROL_OBJS)
	$(AR) -cq $@ $(SMCONTROL_OBJS)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend: 
	$(CC) -MM $(CFLAGS) $(PMCONTROL_SRCS) $(VMCONTROL_SRCS) $(SMCONTROL_SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(PMCONTROL_OBJS) $(VMCONTROL_OBJS) $(SMCONTROL_OBJS) $(TARGETS) *~

-include $(DEPENDS)

