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
SUBDIRS := evm
export SUBDIRS

.PHONY: all
all: subdirs_all

.PHONY: clean
clean: subdirs_clean

.PHONY: install
install: subdirs_install
