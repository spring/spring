AUX_SOURCE_DIRECTORY(libm/flt-32 libm_flt32_source)
AUX_SOURCE_DIRECTORY(libm/dbl-64 libm_dbl64_source)
AUX_SOURCE_DIRECTORY(libm/ldbl-96 libm_ldbl96_source)

# FIXME: Random.cpp fails if it libm/headers is in include directory
# thats why it is set in cxxflags as a workaround
#INCLUDE_DIRECTORIES(libm/headers)

SET(cxxflags "-w -O3 -I${CMAKE_CURRENT_SOURCE_DIR}/libm/headers")
if (NOT $ENV{CXX} MATCHES "icpc")
	SET(cxxflags "${cxxflags} -mfpmath=sse -msse")
endif (NOT $ENV{CXX} MATCHES "icpc")
SET_SOURCE_FILES_PROPERTIES(${libm_flt32_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_FLT32 ${cxxflags}")
SET_SOURCE_FILES_PROPERTIES(${libm_dbl64_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_DBL64 ${cxxflags}")
SET_SOURCE_FILES_PROPERTIES(${libm_ldbl96_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_LDBL96 ${cxxflags}")

ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	SMath.cpp
	Random.cpp
	streflopC.cpp
	${libm_flt32_source}
# not needed => safer (and faster) to not compile them at all
#	${libm_dbl64_source}
#	${libm_ldbl96_source}
)
