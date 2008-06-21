set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/game)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../rts/build/cmake")

IF (MINGW)
	SET(MINGWLIBS ../../mingwlibs CACHE PATH "Location of the mingwlibs")
	INCLUDE_DIRECTORIES(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	
	FIND_PACKAGE(Freetype REQUIRED)
	INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
	
	FIND_PACKAGE(PythonLibs)
	if (PYTHONLIBS_FOUND)
		INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
	else (PYTHONLIBS_FOUND)
		message ("No python libraries found, python bindings disabled")
	endif (PYTHONLIBS_FOUND)
ENDIF (MINGW)

SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb1 -Wall -O1 -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG2 "-ggdb2 -Wall -O0 -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG3 "-ggdb3 -Wall -O0 -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=i686 -DNDEBUG")

ADD_DEFINITIONS( -D_SZ_ONE_DIRECTORY -DNO_AVI -DSYNCCHECK -DUNITSYNC -DBITMAP_NO_OPENGL)

INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

IF (UNIX)
	SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DataDirLocater ../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles ../../rts/System/Platform/Win/WinFileSystemHandler ../../rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)
list(APPEND platformfiles
	../../rts/System/Platform/FileSystem
	../../rts/System/Platform/ConfigHandler)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
set(unitsync_files
	../../rts/System/TdfParser
	../../rts/Game/GameVersion
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Map/MapParser
	../../rts/Rendering/Textures/Bitmap
	../../rts/Rendering/Textures/nv_dds)

if (PYTHONLIBS_FOUND)
	list(APPEND unitsync_files pybind)
endif (PYTHONLIBS_FOUND)

ADD_LIBRARY(unitsync SHARED ${platformfiles} ${unitsync_files} ${fsfiles} unitsync Syncer SyncServer stdafx)
TARGET_LINK_LIBRARIES(unitsync hpiutil2 7zip minizip lua gml GLEW boost_regex-mt IL openal GL GLU IL ILU)
if (PYTHONLIBS_FOUND)
	TARGET_LINK_LIBRARIES(unitsync ${PYTHON_LIBRARIES})
endif (PYTHONLIBS_FOUND)
SET_TARGET_PROPERTIES( unitsync PROPERTIES COMPILE_FLAGS -fPIC)



