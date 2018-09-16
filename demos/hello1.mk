#
# The "evm" project build rules
#
# Copyright (C) 2017-2018 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the "evm" software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the "evm" software project.
#

TARGET := hello1_evm
_INSTDIR_ := $(_INSTALL_PREFIX_)/bin

# Files to be compiled:
SRCS := $(TARGET).c

# include automatic _OBJS_ compilation and SRCSx dependencies generation
include $(_SRCDIR_)/automk/objs.mk

.PHONY: all
all: $(_OBJDIR_)/$(TARGET)

$(_OBJDIR_)/$(TARGET): $(_OBJS_)
	$(CC) $(_OBJS_) -o $@ $(LDFLAGS) -levm -lrt

.PHONY: clean
clean:
	rm -f $(_OBJDIR_)/$(TARGET) $(_OBJDIR_)/$(TARGET).o $(_OBJDIR_)/$(TARGET).d

.PHONY: install
install: $(_INSTDIR_) $(_INSTDIR_)/$(TARGET)

$(_INSTDIR_):
	install -d $@

$(_INSTDIR_)/$(TARGET): $(_OBJDIR_)/$(TARGET)
	install $(_OBJDIR_)/$(TARGET) $@

