#
# The "evm" project build rules
#
# This file is part of the "evm" software project which is
# provided under the Apache license, Version 2.0.
#
#  Copyright 2019 Samo Pogacnik <samo_pogacnik@t-2.net>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

TARGET := libevm.so
_INSTDIR_ := $(_INSTALL_PREFIX_)/lib
#TPATH := $(_BUILDIR_)/$(SUBPATH)
#IPATH := $(_INSTALL_PREFIX_)/lib
HPATH := $(_INSTALL_PREFIX_)/include/evm
HPATH2 := $(_INSTALL_PREFIX_)/include/userlog

# Files to be compiled:
SRCS := evm.c messages.c timers.c
CFLAGS += -fPIC

# include automatic _OBJS_ compilation and SRCS dependencies generation
include $(_SRCDIR_)/automk/objs.mk

.PHONY: all
all: $(_OBJDIR_)/$(TARGET)

$(_OBJDIR_)/$(TARGET): $(_OBJS_)
	$(CC) -shared $(_OBJS_) -o $@

.PHONY: clean
clean:
	rm -f $(_OBJDIR_)/$(TARGET)  $(_OBJDIR_)/*.o $(_OBJDIR_)/*.d

.PHONY: install
install: $(_INSTDIR_) $(_INSTDIR_)/$(TARGET) $(HPATH) $(HPATH)/libevm.h $(HPATH2) $(HPATH2)/log_module.h

$(_INSTDIR_):
	install -d $@

$(_INSTDIR_)/$(TARGET): $(_OBJDIR_)/$(TARGET)
	install $(_OBJDIR_)/$(TARGET) $@

$(HPATH):
	install -d $@

$(HPATH)/libevm.h:\
 $(_SRCDIR_)/include/evm/libevm.h
	install $< $@

$(HPATH2):
	install -d $@

$(HPATH2)/log_module.h:\
 $(_SRCDIR_)/include/userlog/log_module.h
	install $< $@

