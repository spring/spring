INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(SQUISH_SRCS
	alpha.cpp
	clusterfit.cpp
	colourblock.cpp
	colourfit.cpp
	colourset.cpp
	maths.cpp
	rangefit.cpp
	singlecolourfit.cpp
	squish.cpp
)

ADD_LIBRARY(squish STATIC ${SQUISH_SRCS})
SET_TARGET_PROPERTIES(squish PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

