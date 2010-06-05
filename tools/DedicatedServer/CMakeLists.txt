ADD_DEFINITIONS    (-DDEDICATED ${PIC_FLAG} -DNOT_USING_CREG)
REMOVE_DEFINITIONS (-DSTREFLOP_SSE -DAVI_CAPTURING)

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
LIST(APPEND engineDedicatedLibraries ${SDL_LIBRARY})
LIST(APPEND engineDedicatedLibraries ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})
LIST(APPEND engineDedicatedLibraries 7zip minizip lua)



### Assemble the incude dirs
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/System/)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/System/Net/)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/lib/lua/include/)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/lib/7zip)
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})



### Assemble sources
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Net/ net_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/FileSystem/ fs_files)
SET(system_files
	${fs_files}
	${net_files}
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Misc
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
IF (WIN32)
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



ADD_LIBRARY(springserver SHARED ${engineDedicatedSources})
TARGET_LINK_LIBRARIES(springserver ${engineDedicatedLibraries})
IF    (MINGW)
	TARGET_LINK_LIBRARIES (springserver ws2_32)
ELSE  (MINGW)
	SET_TARGET_PROPERTIES(springserver PROPERTIES COMPILE_FLAGS "-fvisibility=default")
ENDIF (MINGW)


ADD_EXECUTABLE(spring-dedicated main)
TARGET_LINK_LIBRARIES(spring-dedicated springserver)


INSTALL(TARGETS springserver     DESTINATION ${LIBDIR})
INSTALL(TARGETS spring-dedicated DESTINATION ${BINDIR})
