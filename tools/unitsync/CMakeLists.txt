
# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

FIND_PACKAGE(Freetype REQUIRED)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})

FIND_PACKAGE(GLEW REQUIRED)
set (unitsync_libs ${GLEW_LIBRARIES} ${DEVIL_IL_LIBRARY} ${DEVIL_ILU_LIBRARY} ${GL_gl_LIBRARY} ${GL_glu_LIBRARY})

FIND_PACKAGE(PythonLibs)
if (PYTHONLIBS_FOUND)
	INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
	list (APPEND unitsync_libs ${PYTHON_LIBRARIES})
else (PYTHONLIBS_FOUND)
	message ("No python libraries found, python bindings disabled")
endif (PYTHONLIBS_FOUND)

ADD_DEFINITIONS( -DUNITSYNC -DBITMAP_NO_OPENGL ${PIC_FLAG})

INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
set(unitsync_files
	../../rts/System/Platform/Misc
	../../rts/System/ConfigHandler
	../../rts/System/LogOutput
	../../rts/System/TdfParser
	../../rts/System/Info
	../../rts/System/Option
	../../rts/Sim/Misc/SideParser
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
ADD_LIBRARY(unitsync SHARED ${unitsync_files} ${fsfiles} unitsync LuaParserAPI Syncer stdafx)
TARGET_LINK_LIBRARIES(unitsync ${unitsync_libs} hpiutil2 7zip minizip lua ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${DEVIL_ILU_LIBRARY} ${SDL_LIBRARY})
if (MINGW)
	set_target_properties(unitsync PROPERTIES LINK_FLAGS -Wl,--add-stdcall-alias)
endif (MINGW)
if (PYTHONLIBS_FOUND)
	TARGET_LINK_LIBRARIES(unitsync ${PYTHON_LIBRARIES})
endif (PYTHONLIBS_FOUND)
install (TARGETS unitsync DESTINATION ${LIBDIR})



