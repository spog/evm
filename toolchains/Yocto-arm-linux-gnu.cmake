# The name of the target system
SET(CMAKE_SYSTEM_NAME Linux)

# The C&C++ cross compiler
SET(CMAKE_C_COMPILER /home/samo/WANDBOARD-yocto/cross/arm-poky-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER /home/samo/WANDBOARD-yocto/cross/arm-poky-linux-gnueabi-g++)

# The target system environment on a host 
SET(CMAKE_FIND_ROOT_PATH /home/samo/WANDBOARD-yocto/fsl-community-bsp/build/tmp/sysroots/wandboard-dual)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

