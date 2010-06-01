
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

