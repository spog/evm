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
CONFFILE := conf.mk
_BUILD_PREFIX_ := $(_BUILDIR_)$(PREFIX)
_INSTALL_PREFIX_ := $(DESTDIR)$(PREFIX)
export SHELL MAKEFILE CONFFILE _BUILD_PREFIX_ _INSTALL_PREFIX_

TOPDIR := $(TOPDIR)/$(SUBDIR)

.PHONY: all
all:

.PHONY: conf
conf: $(MAKEFILE)
	@$(_SRCDIR_)/default.sh subdirs_conf $(TOPDIR)

$(MAKEFILE):
	@echo "Generating $(TOPDIR)/$@"
	@$(_SRCDIR_)/default.sh generate_makefile $(SUBDIR)

