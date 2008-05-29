AUX_SOURCE_DIRECTORY(libm/flt-32 libm_flt32_source)
AUX_SOURCE_DIRECTORY(libm/dbl-64 libm_dbl64_source)
AUX_SOURCE_DIRECTORY(libm/ldbl-96 libm_ldbl96_source)

# FIXME: Random.cpp fails if it libm/headers is in include directory
# th<ts why it is set in cxxflags as a workaround
#INCLUDE_DIRECTORIES(libm/headers)

SET(cxxflags "-w -O3 -frounding-math -fsignaling-nans -fno-strict-aliasing -mieee-fp -Wall -mfpmath=387 -I${CMAKE_CURRENT_SOURCE_DIR}/libm/headers")
SET_SOURCE_FILES_PROPERTIES(${libm_flt32_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_FLT32 ${cxxflags}")
SET_SOURCE_FILES_PROPERTIES(${libm_dbl64_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_DBL64 ${cxxflags}")
SET_SOURCE_FILES_PROPERTIES(${libm_ldbl96_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_LDBL96 ${cxxflags}")

ADD_LIBRARY(streflop STATIC
	SMath.cpp
	Random.cpp
	${libm_flt32_source}
# not needed => safer (and faster) to not compile them at all
#	${libm_dbl64_source}
#	${libm_ldbl96_source}
)
