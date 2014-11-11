#
# The EVM project build rules
#
# Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the EVM software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the EVM software project.
#

# The name of this project is "EVM". CMakeLists files in this project can
# refer to the root source directory of the project as ${EVM_SOURCE_DIR} and
# to the root binary directory of the project as ${EVM_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8)
project (EVM)

# Make sure the compiler can find isolated public include files of our evm library.
include_directories (${EVM_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1")

# Recurse into the "libs/evm" and "demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (libs/evm)
add_subdirectory (demos) 
