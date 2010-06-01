# Spring supplied CMake build file

AUX_SOURCE_DIRECTORY(libm/flt-32 libm_flt32_source)

SET(cxxflags "-w -O3 -I${CMAKE_CURRENT_SOURCE_DIR}/libm/headers")
if (NOT $ENV{CXX} MATCHES "icpc")
	SET(cxxflags "${cxxflags} -mfpmath=sse -msse")
endif (NOT $ENV{CXX} MATCHES "icpc")
SET_SOURCE_FILES_PROPERTIES(${libm_flt32_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_FLT32 ${cxxflags}")

ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	SMath.cpp
	Random.cpp
	streflopC.cpp
	${libm_flt32_source}
)
set_target_properties(streflop PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
#TODO do not use -fPIC for streflop (decreases performance)
