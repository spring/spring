ADD_DEFINITIONS(-DSTREFLOP_SSE) # should be in ../../CMakeLists.txt
ADD_DEFINITIONS(-DDEDICATED ${PIC_FLAG} -DNO_AVI)

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/System/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

IF (UNIX)
	SET(platformfiles
		../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles
		../../rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(system_files 
	${fsfiles}
	${platformfiles}
	 ${netfiles}
	../../rts/System/TdfParser
	../../rts/System/Platform/ConfigHandler
	../../rts/System/LogOutput
	../../rts/System/BaseNetProtocol
	../../rts/System/Demo
	../../rts/System/DemoReader
	../../rts/System/AutohostInterface
	../../rts/System/UnsyncedRNG )

AUX_SOURCE_DIRECTORY(../../rts/Game/Server serverfiles)
ADD_LIBRARY(springserver SHARED
	${system_files}
	${serverfiles}
	../../rts/Game/GameServer
	../../rts/Game/GameSetup
	../../rts/Game/GameData
	../../rts/Game/PlayerBase
	../../rts/Game/GameVersion
	../../rts/Game/CommandMessage
	../../rts/Game/ChatMessage
	../../rts/Game/Console
	../../rts/Game/Action
	../../rts/Lua/LuaIO
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Map/MapParser
	../../rts/Rendering/Textures/TAPalette)
TARGET_LINK_LIBRARIES(springserver ${SDL_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} hpiutil2 7zip minizip lua)
if (MINGW)
	TARGET_LINK_LIBRARIES (springserver ws2_32)
else (MINGW)
	ADD_DEFINITIONS (-fvisibility=default ) #overwrite hidden visibility
endif (MINGW)

ADD_EXECUTABLE(spring-dedicated main)
TARGET_LINK_LIBRARIES(spring-dedicated springserver)

install (TARGETS springserver spring-dedicated RUNTIME DESTINATION ${BINDIR} LIBRARY DESTINATION ${LIBDIR})
