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

SUBDIRS := libs demos
#SUBDIRS := libs
export SUBDIRS

CC := gcc
CFLAGS := -I$(_SRCDIR_)/include -DEVM_VERSION_MAJOR=$(comp_version_MAJOR) -DEVM_VERSION_MINOR=$(comp_version_MINOR) -DEVM_VERSION_PATCH=$(comp_version_PATCH) -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1
LDFLAGS := -L$(_BUILDIR_)/libs/evm
export CC CFLAGS LDFLAGS

.PHONY: all
all: subdirs_all

.PHONY: clean
clean: subdirs_clean

.PHONY: install
install: subdirs_install

