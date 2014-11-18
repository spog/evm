#
# The LIBEVM library build rules
#
# Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the EVM software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the EVM software project.
#

# Create a library called "evm" which includes the source files "evm.c, ...".
# The extension is already found. Any number of sources could be listed here.
add_library (evm SHARED evm.c evm.h messages.c messages.h timers.c timers.h) 

set_target_properties (evm PROPERTIES VERSION ${evm_VERSION_STRING} SOVERSION ${evm_VERSION_MAJOR})

install(TARGETS evm DESTINATION lib COMPONENT libs)
install(FILES ${evm_SOURCE_DIR}/include/evm/libevm.h ${evm_SOURCE_DIR}/include/evm/log_module.h DESTINATION include/evm COMPONENT headers)
