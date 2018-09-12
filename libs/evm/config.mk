#
# The "u2up-build" project build rules
#
# Copyright (C) 2017-2018 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the "u2up-build" software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the "u2up-build" software project.
#

##
# Subdirs to handle:
##
SUBDIRS :=
export SUBDIRS

TARGET := libevm.so
_INSTDIR_ := $(_INSTALL_PREFIX_)/lib
#TPATH := $(_BUILDIR_)/$(SUBPATH)
#IPATH := $(_INSTALL_PREFIX_)/lib
HPATH := $(_INSTALL_PREFIX_)/include/evm

# Files to be compiled:
SRCS := evm.c messages.c timers.c
CFLAGS += -fPIC

# include automatic _OBJS_ compilation and SRCS dependencies generation
include $(_SRCDIR_)/defaults/objs.mk

.PHONY: all
all: $(_OBJDIR_)/$(TARGET)

#$(TPATH)/$(TARGET):\
# $(TPATH)/evm.o\
# $(TPATH)/messages.o\
# $(TPATH)/timers.o
#	$(CC) -shared $^ -o $@
#
#$(TPATH)/evm.o:\
# evm.c\
# evm.h\
# messages.h\
# timers.h
#	libtool -v --mode=compile $(CC) -g -O -c $(CFLAGS) $< -o $@
#
##	$(CC) -c -fPIC $(CFLAGS) $< -o $@
#
#$(TPATH)/messages.o:\
# messages.c\
# messages.h
#	$(CC) -c -fPIC $(CFLAGS) $< -o $@
#
#$(TPATH)/timers.o:\
# timers.c\
# timers.h
#	$(CC) -c -fPIC $(CFLAGS) $< -o $@

$(_OBJDIR_)/$(TARGET): $(_OBJS_)
	$(CC) -shared $(_OBJS_) -o $@

.PHONY: clean
clean:
	rm -f $(_OBJDIR_)/$(TARGET)  $(_OBJDIR_)/*.o $(_OBJDIR_)/*.d

.PHONY: install
install: $(_INSTDIR_) $(_INSTDIR_)/$(TARGET) $(HPATH) $(HPATH)/libevm.h

$(_INSTDIR_):
	install -d $@

$(_INSTDIR_)/$(TARGET): $(_OBJDIR_)/$(TARGET)
	install $(_OBJDIR_)/$(TARGET) $@

$(HPATH):
	install -d $@

$(HPATH)/libevm.h:\
 $(_SRCDIR_)/include/evm/libevm.h
	install $< $@

