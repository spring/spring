# Spring supplied CMake build file

SET(miniZipSources
		"unzip"
		"zip"
		"ioapi"
	)
IF    (WIN32)
	LIST(APPEND miniZipSources "iowin32")
ENDIF (WIN32)

FIND_PACKAGE(ZLIB REQUIRED)

ADD_LIBRARY(minizip STATIC EXCLUDE_FROM_ALL ${miniZipSources})
TARGET_LINK_LIBRARIES(minizip ${ZLIB_LIBRARY})
SET_TARGET_PROPERTIES(minizip PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

