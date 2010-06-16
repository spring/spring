# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/my/sub/dir/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

SET(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")

INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR})
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/System)

SET(demoToolSpringSources
	${ENGINE_SRC_ROOT_DIR}/Game/GameVersion
	${ENGINE_SRC_ROOT_DIR}/Game/PlayerStatistics
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamStatistics
	${ENGINE_SRC_ROOT_DIR}/System/Net/RawPacket
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoReader
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/Demo)

ADD_EXECUTABLE(demotool EXCLUDE_FROM_ALL DemoTool ${demoToolSpringSources})
IF (MINGW)
	# To enable console output/force a console window to open
	SET_TARGET_PROPERTIES(demotool PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
ENDIF (MINGW)
TARGET_LINK_LIBRARIES(demotool ${Boost_PROGRAM_OPTIONS_LIBRARY})


