# Spring supplied CMake build file

Set(MINIZIP_FIND_QUIETLY TRUE)
Find_Package(MiniZip)

If     (MINIZIP_FOUND)
	# Use the system supplied MiniZip library
	Message(STATUS "Using minizip library: ${MINIZIP_LIBRARY}")

	SetGlobal(SPRING_MINIZIP_INCLUDE_DIR "${MINIZIP_INCLUDE_DIR}")
	SetGlobal(SPRING_MINIZIP_LIBRARY "${MINIZIP_LIBRARY}")
Else   (MINIZIP_FOUND)
	# Build our own minizip library
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

	SetGlobal(SPRING_MINIZIP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/rts/lib")
	SetGlobal(SPRING_MINIZIP_LIBRARY minizip)
EndIf  (MINIZIP_FOUND)
