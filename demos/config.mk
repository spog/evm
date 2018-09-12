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

TARGET1 := hello1_evm
TARGET2 := hello2_evm
TARGET3 := hello3_evm
TARGET4 := hello4_evm
_INSTDIR_ := $(_INSTALL_PREFIX_)/bin

# Files to be compiled:
SRCS1 := $(TARGET1).c
SRCS2 := $(TARGET2).c
SRCS3 := $(TARGET3).c
SRCS4 := $(TARGET4).c


# include automatic _OBJS_ compilation and SRCSx dependencies generation
include $(_SRCDIR_)/defaults/objs.mk

.PHONY: all
all:\
 $(_OBJDIR_)/$(TARGET1)\
 $(_OBJDIR_)/$(TARGET2)\
 $(_OBJDIR_)/$(TARGET3)\
 $(_OBJDIR_)/$(TARGET4)

$(_OBJDIR_)/$(TARGET1): $(_OBJS1_)
	$(CC) $(_OBJS1_) -o $@ $(LDFLAGS) -levm -lrt

$(_OBJDIR_)/$(TARGET2): $(_OBJS2_)
	$(CC) $(_OBJS2_) -o $@ $(LDFLAGS) -levm -lrt

$(_OBJDIR_)/$(TARGET3): $(_OBJS3_)
	$(CC) $(_OBJS3_) -o $@ $(LDFLAGS) -levm -lrt -lpthread

$(_OBJDIR_)/$(TARGET4): $(_OBJS4_)
	$(CC) $(_OBJS4_) -o $@ $(LDFLAGS) -levm -lrt -lpthread

.PHONY: clean
clean:
	rm -f $(_OBJDIR_)/$(TARGET4) $(_OBJDIR_)/$(TARGET3) $(_OBJDIR_)/$(TARGET2) $(_OBJDIR_)/$(TARGET1)
	rm -f $(_OBJDIR_)/*.o $(_OBJDIR_)/*.d

.PHONY: install
install:\
 $(_INSTDIR_)\
 $(_INSTDIR_)/$(TARGET1)\
 $(_INSTDIR_)/$(TARGET2)\
 $(_INSTDIR_)/$(TARGET3)\
 $(_INSTDIR_)/$(TARGET4)

$(_INSTDIR_):
	install -d $@

$(_INSTDIR_)/$(TARGET1): $(_OBJDIR_)/$(TARGET1)
	install $(_OBJDIR_)/$(TARGET1) $@

$(_INSTDIR_)/$(TARGET2): $(_OBJDIR_)/$(TARGET2)
	install $(_OBJDIR_)/$(TARGET2) $@

$(_INSTDIR_)/$(TARGET3): $(_OBJDIR_)/$(TARGET3)
	install $(_OBJDIR_)/$(TARGET3) $@

$(_INSTDIR_)/$(TARGET4): $(_OBJDIR_)/$(TARGET4)
	install $(_OBJDIR_)/$(TARGET4) $@

