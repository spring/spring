SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
SET(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)
SET(WINDRES i586-mingw32msvc-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/lib/gcc/i586-mingw32msvc)
