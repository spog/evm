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
set (evm_VERSION_PATCH 2)
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
if (NOT CPACK_GENERATOR)
	message(STATUS "Detecting system NATIVE SW packaging (CPACK_GENERATOR not manually set)...")
	find_program(DPKG_EXECUTABLE dpkg)
	execute_process (COMMAND /bin/sh -o pipefail -c "${DPKG_EXECUTABLE} -l 2> /dev/null | wc -l" RESULT_VARIABLE RV OUTPUT_VARIABLE PACKAGE_COUNTER)
	#message(STATUS "${DPKG_EXECUTABLE} RV=${RV}")
	#message(STATUS "debs PACKAGE_COUNTER=${PACKAGE_COUNTER}")
	if (RV EQUAL 0)
		if (PACKAGE_COUNTER GREATER "0")
			set (CPACK_GENERATOR "DEB")
			message(STATUS "DEB based system: CPACK_GENERATOR set to DEB!")
		endif ()
	endif ()
	find_program(RPM_EXECUTABLE rpm)
	execute_process (COMMAND /bin/sh -o pipefail -c "${RPM_EXECUTABLE} -qa 2> /dev/null | wc -l" RESULT_VARIABLE RV OUTPUT_VARIABLE PACKAGE_COUNTER)
	#message(STATUS "${RPM_EXECUTABLE} RV=${RV}")
	#message(STATUS "rpms PACKAGE_COUNTER=${PACKAGE_COUNTER}")
	if (RV EQUAL 0)
		if (PACKAGE_COUNTER GREATER "0")
			set (CPACK_GENERATOR "RPM")
			message(STATUS "RPM based system: CPACK_GENERATOR set to RPM!")
		endif ()
	endif ()
endif ()

if (CMAKE_TOOLCHAIN_FILE)
	set (CPACK_GENERATOR "TGZ")
	set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
endif ()
#set (CPACK_COMPONENTS_ALL libs headers demos)
#set (CPACK_COMPONENT_HEADERS_DEPENDS libs)
#set (CPACK_COMPONENT_DEMOS_DEPENDS libs)
#set (CPACK_COMPONENT_LIBS_GROUP "Core")
#set (CPACK_COMPONENT_HEADERS_GROUP "Devel")
#set (CPACK_COMPONENT_DEMOS_GROUP "Demos")
#set (CPACK_DEB_COMPONENT_INSTALL ON)
#set (CPACK_RPM_COMPONENT_INSTALL ON)
set (CPACK_PACKAGE_CONTACT "Samo Pogacnik")
set (CPACK_PACKAGE_VENDOR "http://www.u2up.net")
#set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Samo Pogacnik")
#set (CPACK_RPM_PACKAGE_LICENSE "BSD 3-Clause license")

# Generate cpackoptions.cmake at build time to set per generator package filename.
add_custom_target(first ALL
    COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -Dproj_name=${CMAKE_PROJECT_NAME}
    -Dproj_version=${evm_VERSION_STRING}
    -Dtarget_arch=${CMAKE_SYSTEM_PROCESSOR}
    -P ${CMAKE_SOURCE_DIR}/create-cpackoptions.cmake
    )

#add_dependencies(first)

set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_BINARY_DIR}/CPackOptions.cmake)

include (CPack)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

