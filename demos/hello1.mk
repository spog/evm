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

TARGET := hello1_evm
_INSTDIR_ := $(_INSTALL_PREFIX_)/bin

# Files to be compiled:
SRCS := $(TARGET).c

# include automatic _OBJS_ compilation and SRCSx dependencies generation
include $(_SRCDIR_)/automk/objs.mk

.PHONY: all
all: $(_OBJDIR_)/$(TARGET)

$(_OBJDIR_)/$(TARGET): $(_OBJS_)
	$(CC) $(_OBJS_) -o $@ $(LDFLAGS) -levm -lrt -Wl,-rpath=../lib

.PHONY: clean
clean:
	rm -f $(_OBJDIR_)/$(TARGET) $(_OBJDIR_)/$(TARGET).o $(_OBJDIR_)/$(TARGET).d

.PHONY: install
install: $(_INSTDIR_) $(_INSTDIR_)/$(TARGET)

$(_INSTDIR_):
	install -d $@

$(_INSTDIR_)/$(TARGET): $(_OBJDIR_)/$(TARGET)
	install $(_OBJDIR_)/$(TARGET) $@

