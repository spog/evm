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

set_target_properties (evm PROPERTIES VERSION ${EVM_VERSION_STRING} SOVERSION ${EVM_VERSION_MAJOR})

install(TARGETS evm DESTINATION lib)
install(FILES ${EVM_SOURCE_DIR}/include/evm/libevm.h DESTINATION include/evm)
