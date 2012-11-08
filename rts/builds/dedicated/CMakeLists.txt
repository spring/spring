ADD_DEFINITIONS    (-DDEDICATED ${PIC_FLAG} -DNOT_USING_CREG)
ADD_DEFINITIONS    (-DHEADLESS -DNO_SOUND)
REMOVE_DEFINITIONS (-DSTREFLOP_SSE -DAVI_CAPTURING)
REMOVE_DEFINITIONS (-DTRACE_SYNC -DSYNCDEBUG)

# deactivate signaling-NANs for this library
RemoveFlag("-fsignaling-nans")

# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

SET(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")


### Assemble libraries
LIST(APPEND engineDedicatedLibraries ${Boost_REGEX_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY})
LIST(APPEND engineDedicatedLibraries lua archives)
LIST(APPEND engineDedicatedLibraries headlessStubs)
IF    (UNIX)
	# Needed for dynamically loading shared libraries (on some OS)
	LIST(APPEND engineDedicatedLibraries dl)
ENDIF (UNIX)

IF    (MINGW OR APPLE)
	# Windows:
	# We still need these header files,
	# even if we are not going to link with SDL.
	# We have them available anyway (mingwlibs).
	# OS X:
	# Cocoa requires the SDL libary, whenever the SDL headers are used,
	# due to some #define magic, which is practically impossible to workaround.
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
ELSE  (MINGW OR APPLE)
	# Use a direct copy of the GL and SDL headers,
	# as these may not be available on headless systems.
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/SDL)
ENDIF (MINGW OR APPLE)


### Assemble the incude dirs
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/lib/lua/include/)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/lib/7zip)



### Assemble sources
IF	(UNIX)
	SET(sources_engine_Platform_CrashHandler
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Threading
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux/thread_backtrace
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux/CrashHandler)
ELSEIF	(WIN32)
	SET(sources_engine_Platform_CrashHandler
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/seh
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/CrashHandler)
ELSE	()
	SET(sources_engine_Platform_CrashHandler
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Threading
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Mac/CrashHandler)
ENDIF	()

SET(system_files
	${sources_engine_System_FileSystem}
	${sources_engine_System_Log}
	${sources_engine_System_Net}
	${sources_engine_Platform_CrashHandler}
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigHandler
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigLocater
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigSource
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigVariable
	${ENGINE_SRC_ROOT_DIR}/System/CRC
	${ENGINE_SRC_ROOT_DIR}/System/Platform/errorhandler
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Misc
	${ENGINE_SRC_ROOT_DIR}/System/Platform/CmdLineParams
	${ENGINE_SRC_ROOT_DIR}/System/Platform/ScopedFileLock
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Threading
	${ENGINE_SRC_ROOT_DIR}/System/TdfParser
	${ENGINE_SRC_ROOT_DIR}/System/GlobalConfig
	${ENGINE_SRC_ROOT_DIR}/System/Info
	${ENGINE_SRC_ROOT_DIR}/System/LogOutput
	${ENGINE_SRC_ROOT_DIR}/System/TimeUtil
	${ENGINE_SRC_ROOT_DIR}/System/BaseNetProtocol
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/Demo
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoReader
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoRecorder
	${ENGINE_SRC_ROOT_DIR}/System/AutohostInterface
	${ENGINE_SRC_ROOT_DIR}/System/SafeCStrings
	${ENGINE_SRC_ROOT_DIR}/System/UnsyncedRNG
	${ENGINE_SRC_ROOT_DIR}/System/Util
	${ENGINE_SRC_ROOT_DIR}/System/float4
	${ENGINE_SRC_ROOT_DIR}/System/Log/Backend.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFilter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFormatter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/FramePrefixer.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogSinkHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogUtil.c
	${ENGINE_SRC_ROOT_DIR}/System/Log/ConsoleSink.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Misc/SpringTime.cpp
	)
IF    (WIN32)
	LIST(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/WinVersion)
ENDIF (WIN32)

SET(engineDedicatedSources
	${system_files}
	${sources_engine_Game_Server}
	${sources_engine_System_Log}
	${ENGINE_SRC_ROOT_DIR}/Game/GameServer
	${ENGINE_SRC_ROOT_DIR}/Game/ClientSetup
	${ENGINE_SRC_ROOT_DIR}/Game/GameSetup
	${ENGINE_SRC_ROOT_DIR}/Game/GameData
	${ENGINE_SRC_ROOT_DIR}/Game/PlayerBase
	${ENGINE_SRC_ROOT_DIR}/Game/PlayerStatistics
	${ENGINE_SRC_ROOT_DIR}/Game/GameVersion
	${ENGINE_SRC_ROOT_DIR}/Game/IVideoCapturing
	${ENGINE_SRC_ROOT_DIR}/Game/DummyVideoCapturing
	${ENGINE_SRC_ROOT_DIR}/Game/CommandMessage
	${ENGINE_SRC_ROOT_DIR}/Game/ChatMessage
	${ENGINE_SRC_ROOT_DIR}/Game/Action
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamBase
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamStatistics
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/AllyTeam
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaIO
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaParser
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaUtils
	${ENGINE_SRC_ROOT_DIR}/Map/MapParser
	)


# Compile dedicated server shared library
ADD_LIBRARY(springserver-static STATIC EXCLUDE_FROM_ALL ${engineDedicatedSources})
Add_Dependencies(springserver-static generateVersionFiles)
TARGET_LINK_LIBRARIES(springserver-static ${engineDedicatedLibraries})
IF    (MINGW)
	TARGET_LINK_LIBRARIES (springserver-static ${WS2_32_LIBRARY} ${IMAGEHLP_LIBRARY})
ENDIF (MINGW)

ADD_LIBRARY(springserver SHARED
		${ENGINE_SRC_ROOT_DIR}/Map/MapParser
		${sources_engine_System_Log_sinkFile}
		${sources_engine_System_Log_sinkOutputDebugString}
	)
TARGET_LINK_LIBRARIES(springserver springserver-static)

# Compile dedicated server executable
ADD_EXECUTABLE(engine-dedicated
		${ENGINE_SRC_ROOT_DIR}/Map/MapParser
		${sources_engine_System_Log_sinkConsole}
		${sources_engine_System_Log_sinkFile}
		${sources_engine_System_Log_sinkOutputDebugString}
		main
	)
TARGET_LINK_LIBRARIES(engine-dedicated springserver-static)

### Install the executable
INSTALL(TARGETS springserver     DESTINATION ${LIBDIR})
INSTALL(TARGETS engine-dedicated DESTINATION ${BINDIR})
SET_TARGET_PROPERTIES(engine-dedicated PROPERTIES OUTPUT_NAME "spring-dedicated")


# Only build & install spring-dedicated executable & dependencies
# use cases:
# * make spring-dedicated
# * make install-spring-dedicated
set(springDsDeps
		engine-dedicated
		springserver
		gamedata
		unitsync
	)
add_custom_target(spring-dedicated
	WORKING_DIRECTORY
		"${CMAKE_BINARY_DIR}"
	COMMENT
		"  spring-dedicated: Building all dependencies ..." VERBATIM
	)
add_dependencies(spring-dedicated ${springDsDeps}) # This also works for custom targets

set(springDsInstallDirs
		"rts/builds/dedicated"
		"tools/unitsync"
		"cont"
	)
CreateInstallTarget(spring-dedicated springDsDeps springDsInstallDirs)
