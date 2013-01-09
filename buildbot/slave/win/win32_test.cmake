SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER i686-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-mingw32-g++)
SET(WINDRES i686-mingw32-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH
	/usr/lib/gcc/i686-mingw32
	/usr/i686-mingw32
)
