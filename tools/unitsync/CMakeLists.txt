# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

LIST(APPEND unitsync_libs ${DEVIL_IL_LIBRARY})
#LIST(APPEND unitsync_libs ${DEVIL_ILU_LIBRARY})
#LIST(APPEND unitsync_libs ${SDL_LIBRARY})
LIST(APPEND unitsync_libs ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})
LIST(APPEND unitsync_libs 7zip minizip lua)
LIST(APPEND unitsync_libs headlessStubs)

ADD_DEFINITIONS(-DUNITSYNC -DBITMAP_NO_OPENGL ${PIC_FLAG} -DNOT_USING_CREG -DHEADLESS)

INCLUDE_DIRECTORIES(../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/lib/7zip)
INCLUDE_DIRECTORIES(../../rts)
INCLUDE_DIRECTORIES(../../rts/System)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fs_files)
set(main_files
	../../rts/System/Platform/Misc
	../../rts/System/ConfigHandler
	../../rts/System/GlobalConfig
	../../rts/System/LogOutput
	../../rts/System/TdfParser
	../../rts/System/Info
	../../rts/System/Option
	../../rts/System/Util
	../../rts/System/float4
	../../rts/Sim/Misc/SideParser
	../../rts/Game/GameVersion
	../../rts/ExternalAI/LuaAIImplHandler
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Lua/LuaIO
	../../rts/Map/MapParser
	../../rts/Map/SMF/SmfMapFile
	../../rts/Rendering/Textures/Bitmap
	../../rts/Rendering/Textures/nv_dds
	)
if (WIN32)
	LIST(APPEND main_files ../../rts/System/Platform/Win/WinVersion)
endif (WIN32)

set(unitsync_files
	${fs_files}
	${main_files}
	unitsync
	LuaParserAPI
	Syncer
	stdafx
	)

ADD_LIBRARY(unitsync MODULE ${unitsync_files})
TARGET_LINK_LIBRARIES(unitsync ${unitsync_libs})
if (MINGW)
	set_target_properties(unitsync PROPERTIES LINK_FLAGS -Wl,--add-stdcall-alias)
endif (MINGW)
install (TARGETS unitsync DESTINATION ${LIBDIR})

Add_Subdirectory(test)
