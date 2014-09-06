# The name of this project is "EVM". CMakeLists files in this project can
# refer to the root source directory of the project as ${EVM_SOURCE_DIR} and
# to the root binary directory of the project as ${EVM_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8)
project (EVM)

# Make sure the compiler can find isolated public include files of our evm library.
include_directories (${EVM_SOURCE_DIR}/include)

# Recurse into the "libs/evm" and "demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (libs/evm)
add_subdirectory (demos) 
