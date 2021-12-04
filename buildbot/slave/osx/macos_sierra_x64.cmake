set(CMAKE_SYSTEM_NAME "Darwin" CACHE STRING "")
set(CMAKE_SYSTEM_VERSION "10.12" CACHE STRING "")
set(TARGET_ARCH "x86_64" CACHE STRING "")

set(CMAKE_C_COMPILER "/usr/local/Cellar/gcc\@6/6.4.0/bin/gcc-6" CACHE STRING "")
set(CMAKE_CXX_COMPILER "/usr/local/Cellar/gcc\@6/6.4.0/bin/g++-6" CACHE STRING "")

set(Boost_USE_STATIC_LIBS  TRUE CACHE BOOL "")

set(PRD_JSONCPP_INTERNAL FALSE CACHE BOOL "")

set(GLEW_INCLUDE_DIR "/usr/local/include/GL" CACHE STRING "")

set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk" CACHE STRING "")

include_directories("/usr/local/include")
link_directories("/usr/local/lib")
