IF (MINGW)
	set (unitsync_libs glew32 opengl32 glu32 devil ilu python25)
	set (PYTHONLIBS_FOUND TRUE) # Python libs are part of the mingwlibs package
	INCLUDE_DIRECTORIES(${MINGWLIBS}/include/python2.5)
	set (JNI_FOUND TRUE)
ELSE (MINGW)
	FIND_PACKAGE(Freetype REQUIRED)
	INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
	
	FIND_PACKAGE(GLEW REQUIRED)
	
	set (unitsync_libs ${GLEW_LIBRARIES} ${DEVIL_IL_LIBRARY} ${OPENAL_LIBRARY} ${GL_gl_LIBRARY} ${GL_glu_LIBRARY})
	list (APPEND unitsync_libs util) # sometimes this is needed (and I have no clue why)
	
	FIND_PACKAGE(PythonLibs)
	if (PYTHONLIBS_FOUND)
		INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
		list (APPEND unitsync_libs ${PYTHON_LIBRARIES})
	else (PYTHONLIBS_FOUND)
		message ("No python libraries found, python bindings disabled")
	endif (PYTHONLIBS_FOUND)
	
	FIND_PACKAGE(JNI)
	if (JAVA_INCLUDE_PATH)
		include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} ${JNI_INCLUDE_DIRS})
		set (JAVA_FOUND TRUE)
	else (JAVA_INCLUDE_PATH)
		message ("No Java includes found, java bindings disabled")
	endif (JAVA_INCLUDE_PATH)
	
ENDIF (MINGW)

ADD_DEFINITIONS( -DUNITSYNC -DBITMAP_NO_OPENGL ${PIC_FLAG})

INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

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
	../../rts/Map/SMF/SmfMapFile
	../../rts/Rendering/Textures/Bitmap
	../../rts/Rendering/Textures/nv_dds)

if (PYTHONLIBS_FOUND)
	list(APPEND unitsync_files pybind)
endif (PYTHONLIBS_FOUND)
if (JAVA_FOUND)
	list(APPEND unitsync_files javabind)
endif (JAVA_FOUND)
ADD_LIBRARY(unitsync SHARED ${platformfiles} ${unitsync_files} ${fsfiles} unitsync LuaParserAPI Syncer SyncServer stdafx)
TARGET_LINK_LIBRARIES(unitsync ${unitsync_libs} hpiutil2 7zip minizip lua ${Boost_REGEX_LIBRARY} ${IL_ILU_LIBRARY} ${SDL_LIBRARY})
if (PYTHONLIBS_FOUND)
	TARGET_LINK_LIBRARIES(unitsync ${PYTHON_LIBRARIES})
endif (PYTHONLIBS_FOUND)
install (TARGETS unitsync DESTINATION ${LIBDIR})



