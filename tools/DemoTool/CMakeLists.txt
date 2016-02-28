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
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/src-generated/engine)

ADD_DEFINITIONS(-DTOOLS)

SET(demoToolSpringSources
	${ENGINE_SRC_ROOT_DIR}/Game/GameVersion.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/Players/PlayerStatistics.cpp
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamStatistics.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/FileHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/FileSystem.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/FileSystemAbstraction.cpp
	${ENGINE_SRC_ROOT_DIR}/System/FileSystem/GZFileHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Util.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Net/RawPacket.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoReader.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/Demo.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/Backend.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFilter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFormatter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/FramePrefixer.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogSinkHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogUtil.c
	${ENGINE_SRC_ROOT_DIR}/System/Log/ConsoleSink.cpp
	${ENGINE_SRC_ROOT_DIR}/System/SafeCStrings.c
)

ADD_EXECUTABLE(demotool EXCLUDE_FROM_ALL DemoTool ${demoToolSpringSources})
IF (MINGW)
	# To enable console output/force a console window to open
	SET_TARGET_PROPERTIES(demotool PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
ENDIF (MINGW)
add_definitions(-DNOT_USING_CREG)
TARGET_LINK_LIBRARIES(demotool
		${Boost_REGEX_LIBRARY}
		${Boost_PROGRAM_OPTIONS_LIBRARY}
		${Boost_SYSTEM_LIBRARY}
		${Boost_FILESYSTEM_LIBRARY}
		${SPRING_MINIZIP_LIBRARY}
	)
Add_Dependencies(demotool generateVersionFiles)


