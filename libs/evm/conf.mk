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

include $(_SRCDIR_)/default.mk

SUBDIRS :=
export SUBDIRS

TARGET := libevm.so
TPATH := $(_BUILDIR_)/$(SUBPATH)
IPATH := $(_INSTALL_PREFIX_)/lib
HPATH := $(_INSTALL_PREFIX_)/include/evm

.PHONY: all
all: $(TPATH)/$(TARGET)

$(TPATH)/$(TARGET):\
 $(TPATH)/evm.o\
 $(TPATH)/messages.o\
 $(TPATH)/timers.o
	$(CC) -shared $^ -o $@

$(TPATH)/evm.o:\
 $(_SRCDIR_)/$(SUBPATH)/evm.c\
 $(_SRCDIR_)/$(SUBPATH)/evm.h\
 $(_SRCDIR_)/$(SUBPATH)/messages.h\
 $(_SRCDIR_)/$(SUBPATH)/timers.h\
 $(TPATH)
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

$(TPATH)/messages.o:\
 $(_SRCDIR_)/$(SUBPATH)/messages.c\
 $(_SRCDIR_)/$(SUBPATH)/messages.h\
 $(TPATH)
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

$(TPATH)/timers.o:\
 $(_SRCDIR_)/$(SUBPATH)/timers.c\
 $(_SRCDIR_)/$(SUBPATH)/timers.h\
 $(TPATH)
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

$(TPATH):
	install -d $@

.PHONY: install
install: $(IPATH) $(IPATH)/$(TARGET) $(HPATH) $(HPATH)/libevm.h

$(IPATH):
	install -d $@

$(IPATH)/$(TARGET):
	install $(TPATH)/$(TARGET) $@

$(HPATH):
	install -d $@

$(HPATH)/libevm.h:\
 $(_SRCDIR_)/include/evm/libevm.h
	install $< $@

