# Make sure the compiler can find isolated public include files of our evm library.
include_directories (${EVM_SOURCE_DIR}/include)

# Create a library called "evm" which includes the source files "evm.c, ...".
# The extension is already found. Any number of sources could be listed here.
add_library (evm SHARED evm.c evm.h messages.c messages.h timers.c timers.h) 

install(TARGETS evm DESTINATION lib)
install(FILES ${EVM_SOURCE_DIR}/include/evm/libevm.h DESTINATION include/evm)
