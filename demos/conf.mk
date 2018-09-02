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

TPATH := $(_BUILDIR_)/$(SUBPATH)
IPATH := $(_INSTALL_PREFIX_)/bin

TARGET1 := hello1_evm
TARGET2 := hello2_evm
TARGET3 := hello3_evm
TARGET4 := hello4_evm

.PHONY: all
all:\
 $(TPATH)/$(TARGET1)\
 $(TPATH)/$(TARGET2)\
 $(TPATH)/$(TARGET3)\
 $(TPATH)/$(TARGET4)

$(TPATH)/$(TARGET1):\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET1).c\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET1).h\
 ../libs/evm/libevm.so\
 $(TPATH)
	$(CC) $(CFLAGS) $< ../libs/evm/libevm.so -lrt -o $@

$(TPATH)/$(TARGET2):\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET2).c\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET2).h\
 ../libs/evm/libevm.so\
 $(TPATH)
	$(CC) $(CFLAGS) $< ../libs/evm/libevm.so -lrt -o $@

$(TPATH)/$(TARGET3):\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET3).c\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET3).h\
 ../libs/evm/libevm.so\
 $(TPATH)
	$(CC) $(CFLAGS) $< ../libs/evm/libevm.so -lrt -lpthread -o $@

$(TPATH)/$(TARGET4):\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET4).c\
 $(_SRCDIR_)/$(SUBPATH)/$(TARGET4).h\
 ../libs/evm/libevm.so\
 $(TPATH)
	$(CC) $(CFLAGS) $< ../libs/evm/libevm.so -lrt -lpthread -o $@

$(TPATH):
	install -d $@

.PHONY: install
install:\
 $(IPATH)\
 $(IPATH)/$(TARGET1)\
 $(IPATH)/$(TARGET2)\
 $(IPATH)/$(TARGET3)\
 $(IPATH)/$(TARGET4)

$(IPATH):
	install -d $@

$(IPATH)/$(TARGET1):
	install $(TPATH)/$(TARGET1) $@

$(IPATH)/$(TARGET2):
	install $(TPATH)/$(TARGET2) $@

$(IPATH)/$(TARGET3):
	install $(TPATH)/$(TARGET3) $@

$(IPATH)/$(TARGET4):
	install $(TPATH)/$(TARGET4) $@

