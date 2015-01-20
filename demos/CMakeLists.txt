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
add_executable (hello1_evm hello1_evm.c hello1_evm.h)    
add_executable (hello2_evm hello2_evm.c hello2_evm.h)    
add_executable (hello3_evm hello3_evm.c hello3_evm.h)    
add_executable (hello4_evm hello4_evm.c hello4_evm.h)    
        
# Make sure the linker can find the Hello library once it is built.  
link_directories (${evm_BINARY_DIR}/evm)

# Link the executable to the evm library.  
target_link_libraries (hello1_evm evm rt)
target_link_libraries (hello2_evm evm rt)
target_link_libraries (hello3_evm evm rt pthread)
target_link_libraries (hello4_evm evm rt pthread)

install(TARGETS hello1_evm hello2_evm hello3_evm hello4_evm RUNTIME DESTINATION bin COMPONENT demos)

