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

TARGET := libevm
TPATH := $(_BUILD_PREFIX_)/lib
IPATH := $(_INSTALL_PREFIX_)/lib

.PHONY: all
all: $(TPATH)/$(TARGET)

$(TPATH)/$(TARGET):\
 $(_SRCDIR_)/$(SUBPATH)/evm.c\
 $(_SRCDIR_)/$(SUBPATH)/evm.h\
 $(_SRCDIR_)/$(SUBPATH)/messages.c\
 $(_SRCDIR_)/$(SUBPATH)/messages.h\
 $(_SRCDIR_)/$(SUBPATH)/timers.c\
 $(_SRCDIR_)/$(SUBPATH)/timers.h\
 $(TPATH)
	$(CC) -o $@ $<

$(TPATH):
	install -d $@

.PHONY: install
install: $(IPATH) $(IPATH)/$(TARGET)

$(IPATH):
	install -d $@

$(IPATH)/$(TARGET):
	install $(TPATH)/$(TARGET) $@

