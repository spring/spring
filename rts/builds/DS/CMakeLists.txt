ADD_DEFINITIONS    (-DDEDICATED ${PIC_FLAG} -DNOT_USING_CREG)
ADD_DEFINITIONS    (-DHEADLESS -DNO_SOUND)
REMOVE_DEFINITIONS (-DSTREFLOP_SSE -DAVI_CAPTURING)
REMOVE_DEFINITIONS (-DUSE_MMGR -DTRACE_SYNC -DSYNCDEBUG)

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
LIST(APPEND engineDedicatedLibraries ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})
LIST(APPEND engineDedicatedLibraries 7zip minizip lua)
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
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Net/ net_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/FileSystem/ fs_files)
SET(system_files
	${fs_files}
	${net_files}
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Misc
	${ENGINE_SRC_ROOT_DIR}/System/Platform/errorhandler
	${ENGINE_SRC_ROOT_DIR}/System/TdfParser
	${ENGINE_SRC_ROOT_DIR}/System/ConfigHandler
	${ENGINE_SRC_ROOT_DIR}/System/GlobalConfig
	${ENGINE_SRC_ROOT_DIR}/System/LogOutput
	${ENGINE_SRC_ROOT_DIR}/System/TimeUtil
	${ENGINE_SRC_ROOT_DIR}/System/BaseNetProtocol
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/Demo
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoReader
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoRecorder
	${ENGINE_SRC_ROOT_DIR}/System/AutohostInterface
	${ENGINE_SRC_ROOT_DIR}/System/UnsyncedRNG
	${ENGINE_SRC_ROOT_DIR}/System/Util
	${ENGINE_SRC_ROOT_DIR}/System/float4)
IF    (WIN32)
	LIST(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/WinVersion)
ENDIF (WIN32)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Game/Server server_files)

SET(engineDedicatedSources
	${system_files}
	${server_files}
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
TARGET_LINK_LIBRARIES(springserver-static ${engineDedicatedLibraries})
IF    (MINGW)
	TARGET_LINK_LIBRARIES (springserver-static ws2_32)
ENDIF (MINGW)

ADD_LIBRARY(springserver SHARED ${ENGINE_SRC_ROOT_DIR}/Map/MapParser)
TARGET_LINK_LIBRARIES(springserver springserver-static)

# Compile dedicated server executable
ADD_EXECUTABLE(spring-dedicated ${ENGINE_SRC_ROOT_DIR}/Map/MapParser main)
TARGET_LINK_LIBRARIES(spring-dedicated springserver-static)


### Install the executable
INSTALL(TARGETS springserver     DESTINATION ${LIBDIR})
INSTALL(TARGETS spring-dedicated DESTINATION ${BINDIR})

# Only install spring-ds executable & dependencies
# use case: make install-spring-ds
set(springDsInstallDirs
		"rts/builds/DS"
		"tools/unitsync"
		"cont"
	)
set(springDsInstallDeps
		spring-dedicated
		springserver
		gamedata
		unitsync
	)
CreateInstallTarget(spring-dedicated springDsInstallDeps springDsInstallDirs)
