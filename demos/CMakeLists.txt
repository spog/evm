#
# The demo programs build rules
#
# Copyright (C) 2014 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the EVM software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the EVM software project.
#

# Add executable called "hello_evm" that is built from the source files  
# "hello_evm.c" and "hello_evm.h".  The extensions are automatically found.  
add_executable (hello_evm hello_evm.c hello_evm.h)    
add_executable (hello2_evm hello2_evm.c hello2_evm.h)    
        
# Make sure the linker can find the Hello library once it is built.  
link_directories (${EVM_BINARY_DIR}/evm)

# Link the executable to the evm library.  
target_link_libraries (hello_evm evm rt)
target_link_libraries (hello2_evm evm rt)

