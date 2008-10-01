IF (MINGW)
	set (unitsync_libs glew32 opengl32 glu32 devil ilu)
ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	
	FIND_PACKAGE(Freetype REQUIRED)
	INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
	
	FIND_PACKAGE(GLEW REQUIRED)
	
	FIND_PACKAGE(PythonLibs)
	if (PYTHONLIBS_FOUND)
		INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
	else (PYTHONLIBS_FOUND)
		message ("No python libraries found, python bindings disabled")
	endif (PYTHONLIBS_FOUND)
	
	set (unitsync_libs ${GLEW_LIBRARIES} IL openal GL GLU)
ENDIF (MINGW)

ADD_DEFINITIONS( -D_SZ_ONE_DIRECTORY -DNO_AVI -DSYNCCHECK -DUNITSYNC -DBITMAP_NO_OPENGL)

INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts ../../rts/Game ../../rts/lib/7zip ../../rts/System)

IF (UNIX)
	SET(platformfiles
		../../rts/System/Platform/Linux/UnixFileSystemHandler
		../../rts/System/Platform/Linux/DataDirLocater
		../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles
		../../rts/System/Platform/Win/WinFileSystemHandler
		../../rts/System/Platform/Linux/DataDirLocater
		../../rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)
list(APPEND platformfiles
	../../rts/System/Platform/FileSystem
	../../rts/System/Platform/ConfigHandler)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
set(unitsync_files
	../../rts/System/TdfParser
	../../rts/Sim/SideParser
	../../rts/Game/GameVersion
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Lua/LuaIO
	../../rts/Map/MapParser
	../../rts/Rendering/Textures/Bitmap
	../../rts/Rendering/Textures/nv_dds)

if (PYTHONLIBS_FOUND)
	list(APPEND unitsync_files pybind)
endif (PYTHONLIBS_FOUND)

ADD_LIBRARY(unitsync SHARED ${platformfiles} ${unitsync_files} ${fsfiles} unitsync LuaParserAPI Syncer SyncServer stdafx)
TARGET_LINK_LIBRARIES(unitsync ${unitsync_libs} hpiutil2 7zip minizip lua boost_regex-mt ILU SDL)
if (PYTHONLIBS_FOUND)
	TARGET_LINK_LIBRARIES(unitsync ${PYTHON_LIBRARIES})
endif (PYTHONLIBS_FOUND)
SET_TARGET_PROPERTIES( unitsync PROPERTIES COMPILE_FLAGS -fPIC)
install (TARGETS unitsync DESTINATION ${LIBDIR})



