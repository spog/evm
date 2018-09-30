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

SUBMAKES := libs | demos
export SUBMAKES

comp_version_MAJOR := 0
comp_version_MINOR := 1
comp_version_PATCH := 2
export  comp_version_MAJOR comp_version_MINOR comp_version_PATCH

CC := gcc
CFLAGS := -I$(_SRCDIR_)/include -DEVM_VERSION_MAJOR=$(comp_version_MAJOR) -DEVM_VERSION_MINOR=$(comp_version_MINOR) -DEVM_VERSION_PATCH=$(comp_version_PATCH) -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1
LDFLAGS := -L$(_BUILDIR_)/libs/evm
export CC CFLAGS LDFLAGS

