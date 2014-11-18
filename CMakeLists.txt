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
project (evm)

# The project version number.
set (evm_VERSION_MAJOR 0)
set (evm_VERSION_MINOR 1)
set (evm_VERSION_PATCH 1)
set (evm_VERSION_STRING ${evm_VERSION_MAJOR}.${evm_VERSION_MINOR}.${evm_VERSION_PATCH})

# The project configuration header file passes required CMake settings
# to the source code.
configure_file (
  "${evm_SOURCE_DIR}/config.h.in"
  "${evm_BINARY_DIR}/config.h"
  )
 
# The search path for generated configuration include files.
include_directories("${evm_BINARY_DIR}")

# Make sure the compiler can find isolated public include files of our evm library.
include_directories (${evm_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DEVMLOG_MODULE_DEBUG=1 -DEVMLOG_MODULE_TRACE=1")

# Recurse into the "libs/evm" and "demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (libs/evm)
add_subdirectory (demos) 

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "The EVM (EVent Machine) software is a C programming framework consisting of the libevm library at its core.")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${evm_SOURCE_DIR}/README")
set (CPACK_RESOURCE_FILE_LICENSE "${evm_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "${evm_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${evm_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${evm_VERSION_PATCH}")
set (CPACK_GENERATOR "DEB" "RPM")
set (CPACK_COMPONENTS_ALL libs headers demos)
set (CPACK_COMPONENT_HEADERS_DEPENDS libs)
set (CPACK_COMPONENT_DEMOS_DEPENDS libs)
set (CPACK_COMPONENT_LIBS_GROUP "Core")
set (CPACK_COMPONENT_HEADERS_GROUP "Devel")
set (CPACK_COMPONENT_DEMOS_GROUP "Demos")
set (CPACK_DEB_COMPONENT_INSTALL ON)
set (CPACK_RPM_COMPONENT_INSTALL ON)
set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Samo Pogacnik")
set (CPACK_RPM_PACKAGE_LICENSE "BSD 3-Clause license")
include (CPack)
