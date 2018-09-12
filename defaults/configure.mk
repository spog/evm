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

SHELL := /bin/bash
MAKEFILE := Makefile
CONFIG_MKFILE := config.mk
BUILD_MKFILE := build.mk
INSTALL_MKFILE := install.mk
PACKAGE_MKFILE := package.mk
export SHELL MAKEFILE CONFIG_MKFILE BUILD_MKFILE INSTALL_MKFILE PACKAGE_MKFILE

SUBPATH := $(shell $(_SRCDIR_)/defaults/default.sh subpath_set)

include $(CONFIG_MKFILE)

.PHONY: configure
configure: $(_SRCDIR_)/.build/$(SUBPATH)/$(MAKEFILE)
	$(_SRCDIR_)/defaults/default.sh subdirs_config $(SUBPATH)

$(_SRCDIR_)/.build/$(SUBPATH)/$(MAKEFILE):
	$(_SRCDIR_)/defaults/default.sh generate_makefile $(SUBPATH)

.PHONY: subdirs_all
subdirs_all:
	$(_SRCDIR_)/defaults/default.sh subdirs_make all

