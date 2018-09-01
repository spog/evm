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

SUBDIRS := evm
export SUBDIRS

##
# Subdirs to handle:
##
SUBDIRS := lib1 lib2
export SUBDIRS

.PHONY: subdirs_all
subdirs_all:
	$(_SRCDIR_)/default.sh subdirs_make all

.PHONY: subdirs_install
subdirs_install:
	$(_SRCDIR_)/default.sh subdirs_make install

