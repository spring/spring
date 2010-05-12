INCLUDE_DIRECTORIES(../../rts)
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

FIND_PACKAGE(Boost 1.34.0 COMPONENTS filesystem REQUIRED)

SET(mySources ArchiveMover)

### Add icon to exe using windres
IF    (MINGW)
	IF    (WINDRES_FOUND)
		ADD_CUSTOM_COMMAND(
			OUTPUT
				"${CMAKE_CURRENT_BINARY_DIR}/icon.o"
			COMMAND
				"${WINDRES_BIN}"
				"-I${CMAKE_CURRENT_SOURCE_DIR}"
				"-i${CMAKE_CURRENT_SOURCE_DIR}/icon.rc" 
				"-o" "${CMAKE_CURRENT_BINARY_DIR}/icon.o"
				"-v"
			)
		LIST(APPEND mySources "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	ELSE  (WINDRES_FOUND)
		MESSAGE(WARNING "Could not find windres, not adding icon to executable.")
	ENDIF (WINDRES_FOUND)
ENDIF (MINGW)

ADD_EXECUTABLE(ArchiveMover ${mySources})

TARGET_LINK_LIBRARIES(ArchiveMover ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} 7zip minizip)


INSTALL (TARGETS ArchiveMover RUNTIME DESTINATION ${BINDIR})
