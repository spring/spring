SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER /usr/bin/i586-mingw32msvc-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/i586-mingw32msvc-g++)
SET(WINDRES /usr/bin/i586-mingw32msvc-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/lib/gcc/i586-mingw32msvc)

# the spring mingw32 dependencies
SET(MINGWLIBS "~/mingwlibs/")

# the path that make install will use to put the final binaries
SET(CMAKE_INSTALL_PREFIX /share/sources/spring-cross/win32/final)
