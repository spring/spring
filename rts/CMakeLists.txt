# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

SET(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")



### Assemble defines
SET(USE_MMGR FALSE CACHE BOOL "Use memory manager?")
if (USE_MMGR)
	ADD_DEFINITIONS(-DUSE_MMGR)
endif (USE_MMGR)

SET(TRACE_SYNC FALSE CACHE BOOL "Enable sync tracker")
if (TRACE_SYNC)
	ADD_DEFINITIONS(-DTRACE_SYNC)
endif (TRACE_SYNC)

SET(SYNCDEBUG FALSE CACHE BOOL "Enable sync debugger (needs SYNCCHECK=true)")
if (SYNCDEBUG)
	ADD_DEFINITIONS(-DSYNCDEBUG)
endif (SYNCDEBUG)

# Only used by GML build, but used in builds/GML and lib/gml
SET(USE_GML_DEBUG FALSE CACHE BOOL "Use GML call debugging?")



### Assemble our incude dirs
INCLUDE_DIRECTORIES(BEFORE lib/lua/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/System/)



### Assemble libraries
FIND_PACKAGE(OpenAL REQUIRED)
LIST(APPEND engineCommonLibraries ${OPENAL_LIBRARY})
FIND_PACKAGE(OggVorbis REQUIRED)
LIST(APPEND engineCommonLibraries ${OGG_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})

# We also still need the header files for headless,
# even if we are not going to link with gl, glu, or glut
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLU REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
LIST(APPEND engineLibraries ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
# We still link with glew for headless too.
# (you try stubbing the whole of libGLEW ;-) Good luck with that :-P)
LIST(APPEND engineCommonLibraries ${GLEW_LIBRARIES} ${DEVIL_IL_LIBRARY})

FIND_PACKAGE(Freetype REQUIRED)
LIST(APPEND engineCommonLibraries ${FREETYPE_LIBRARIES})

### Find include directories and add platform specific libraries
IF (MINGW)
	FIND_PACKAGE(Win32Libs REQUIRED)
	LIST(APPEND engineCommonLibraries ${WIN32_LIBRARIES} mingw32)
ELSE (MINGW)
	FIND_PACKAGE(X11 REQUIRED)
	LIST(APPEND engineCommonLibraries ${X11_X11_LIB} ${X11_Xcursor_LIB})
ENDIF (MINGW)

LIST(APPEND engineCommonLibraries ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_SIGNALS_LIBRARY})
LIST(APPEND engineLibraries ${SDL_LIBRARY})

ADD_SUBDIRECTORY(lib)
LIST(APPEND engineCommonLibraries lua 7zip oscpack minizip streflop lobby)

IF    (UNIX)
	# Needed for dynamically loading shared libraries (on some OS)
	LIST(APPEND engineCommonLibraries dl)
ENDIF (UNIX)

LIST(APPEND engineLibraries ${engineCommonLibraries})



### Assemble external incude dirs
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})



### Assemble engine sources
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Game game_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Game/Camera game_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Game/Server game_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Game/StartScripts game_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Game/UI game_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Menu game_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Lua lua_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/ExternalAI ai_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/ExternalAI/Interface ai_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Map map_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Map/SM3 map_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Map/SM3/terrain map_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Map/SMF map_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Rendering render_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Rendering/Env render_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Rendering/GL render_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Rendering/Shaders render_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Rendering/Textures render_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Rendering/Models render_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/aGui render_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Features sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Misc sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/MoveTypes sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/MoveTypes/MoveMath sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Objects sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Path sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Projectiles sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Projectiles/WeaponProjectiles sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Projectiles/Unsynced sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Units sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Units/COB sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Units/CommandAI sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Units/Groups sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Units/UnitTypes sim_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/Sim/Weapons sim_files)

AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Platform sys_files)
### only use the target platform related directory
IF    (UNIX)
	AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux sys_files)
ELSE  (UNIX)
	AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Platform/Win sys_files)
ENDIF (UNIX)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/FileSystem sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Net sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Script sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Sync sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Sound sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/creg sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/Input sys_files)
AUX_SOURCE_DIRECTORY(${ENGINE_SRC_ROOT_DIR}/System/LoadSave sys_files)

SET(engineSources
		${game_files}
		${lua_files}
		${map_files}
		${fs_files}
		${render_files}
		${sim_files}
		${sys_files}
		${ai_files}
		${nedmalloc_obj}
	)

### Add headers for generated project files (e.g. Code::Blocks)
file(GLOB_RECURSE engineHeaders "*.h" "*.hpp")


### Add icon and manifest to exe using windres
IF    (MINGW)
	SET(ENGINE_ICON_DIR "${ENGINE_SRC_ROOT_DIR}")
	SET(ENGINE_ICON_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
	SET(ENGINE_ICON_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_ICON "${ENGINE_ICON_DIR}" "${ENGINE_ICON_RES}" "${ENGINE_ICON_OBJ}")
ELSE  (MINGW)
	SET(ENGINE_ICON "")
ENDIF (MINGW)


ADD_EXECUTABLE(spring ${engineSources} ${ENGINE_ICON} ${engineHeaders})
TARGET_LINK_LIBRARIES(spring ${engineLibraries})

IF    (MINGW)
	#SET_TARGET_PROPERTIES(spring PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
	#install (FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
ENDIF (MINGW)

INSTALL(TARGETS spring DESTINATION ${BINDIR})

Add_Subdirectory(builds)
