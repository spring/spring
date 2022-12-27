
# based on Boost's tools/build/CMake/CompileTest/CMakeLists.txt
# URL: http://svn.boost.org/svn/boost/branches/CMake/Boost_1_35_0/tools/build/CMake/CompileTest/CMakeLists.txt

# helper of spring_test_compile_fail macro

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

project(CompileFailTest)

string(REPLACE ":" ";" INCLUDE_PATHS ${INCLUDES})
separate_arguments(INCLUDE_PATHS)
target_include_directories (compile ${INCLUDE_PATHS})
add_library(compile STATIC "${SOURCE}")
add_definitions(-DFAIL)
add_definitions(${DEF_FLAG})
