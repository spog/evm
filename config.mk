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

SUBMAKES := libs | demos
export SUBMAKES

comp_version_MAJOR := 0
comp_version_MINOR := 5
comp_version_PATCH := 0
export  comp_version_MAJOR comp_version_MINOR comp_version_PATCH

CFLAGS := -g -Wall -I$(_SRCDIR_)/include -DEVM_VERSION_MAJOR=$(comp_version_MAJOR) -DEVM_VERSION_MINOR=$(comp_version_MINOR) -DEVM_VERSION_PATCH=$(comp_version_PATCH) -DU2UP_LOG_MODULE_DEBUG=1 -DU2UP_LOG_MODULE_TRACE=1
LDFLAGS := -L$(_BUILDIR_)/libs/evm
export CFLAGS LDFLAGS

