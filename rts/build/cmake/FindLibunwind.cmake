# Try to find libunwind
#
# based on: http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries#Using_external_libraries_that_CMake_doesn.27t_yet_have_modules_for
# written by Maj.Boredom
#
# Once done this will define
#   LIBUNWIND_FOUND - System has libunwind
# and optionally,
#   LIBUNWIND_INCLUDE_DIRS - The libunwind include directories
#   LIBUNWIND_LIBRARIES - The libraries needed to use libunwind
#   LIBUNWIND_DEFINITIONS - Compiler switches needed to use libunwind
#

find_package(PkgConfig)

set(LIB_STD_ARGS
      PATH_SUFFIXES
          lib
          lib64
      PATHS
          ${PROJECT_BINARY_DIR}
          ${PROJECT_SOURCE_DIR}
          $ENV{LD_LIBRARY_PATH}
          $ENV{LIBRARY_PATH}
          /usr/lib
          /usr/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib/system
)

find_path(LIBUNWIND_INCLUDE_DIRS libunwind.h)

if (APPLE AND LIBUNWIND_INCLUDE_DIRS)
  # FIXME: OS X 10.10 doesn't have static libunwind.a only dynamic libunwind.dylib;
  #        link with "-framework Cocoa"
  set(LIBUNWIND_LIBRARY "-framework Cocoa")
else ()
  find_library(LIBUNWIND_LIBRARY NAMES unwind ${LIB_STD_ARGS})
endif ()


if (LIBUNWIND_INCLUDE_DIRS AND LIBUNWIND_LIBRARY)
  
  set(LIBUNWIND_FOUND TRUE)
  set(LIBUNWIND_DEFINITIONS "-DLIBUNWIND")
  set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARY})
  
else (LIBUNWIND_INCLUDE_DIRS AND LIBUNWIND_LIBRARY)
  
  set(LIBUNWIND_FOUND FALSE)

endif(LIBUNWIND_INCLUDE_DIRS AND LIBUNWIND_LIBRARY)

