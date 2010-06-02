
ADD_DEFINITIONS(-DUSE_GML)
# USE_GML requires this
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-tls-direct-seg-refs")

# USE_GML_SIM requires USE_GML
ADD_DEFINITIONS(-DUSE_GML_SIM)

if (USE_GML_DEBUG)
	ADD_DEFINITIONS(-DUSE_GML_DEBUG)
endif (USE_GML_DEBUG)


SET(gmlSources
		"ThreadSafeContainers.cpp"
		"gml.cpp"
		"speedy-tls.cpp"
	)

ADD_LIBRARY(gml STATIC EXCLUDE_FROM_ALL ${gmlSources})
if (MINGW)
	TARGET_LINK_LIBRARIES(gml opengl32 glu32)
else (MINGW)
	TARGET_LINK_LIBRARIES(gml GL GLU)
endif (MINGW)

