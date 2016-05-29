# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

SET(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")


option(TRACE_SYNC "Enable sync tracker" FALSE)
if (TRACE_SYNC)
	ADD_DEFINITIONS(-DTRACE_SYNC)
endif (TRACE_SYNC)

option(SYNCDEBUG "Enable sync debugger (needs SYNCCHECK=true)" FALSE)
if (SYNCDEBUG)
	ADD_DEFINITIONS(-DSYNCDEBUG)
	If     (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
		Message(WARNING "It is recommended to set CMAKE_BUILD_TYPE to DEBUG for a SYNCDEBUG build")
	EndIf  ()
	If     (NOT SYNCCHECK)
		Message(FATAL_ERROR "You need SYNCCHECK=TRUE for a SYNCDEBUG build")
	EndIf  ()
	If     (NOT TRACE_SYNC)
		Message(WARNING "It is recommended to use TRACE_SYNC=TRUE for a SYNCDEBUG build")
	EndIf  ()
endif (SYNCDEBUG)

option(DEBUG_GLSTATE "enable GL_STATE_CHECKER" FALSE)
if(DEBUG_GLSTATE)
	add_definitions(-DDEBUG_GLSTATE)
endif()

### give error when not found
FIND_PACKAGE_STATIC(DevIL REQUIRED)

### Assemble common incude dirs
INCLUDE_DIRECTORIES(BEFORE lib/lua/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/AL)
INCLUDE_DIRECTORIES(${SPRING_MINIZIP_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${IL_INCLUDE_DIR})

### Assemble common libraries
Add_Subdirectory(System/Sound)
if    (NO_SOUND)
	ADD_DEFINITIONS(-DNO_SOUND)
endif (NO_SOUND)

### Find include directories and add platform specific libraries
IF    (UNIX AND NOT MINGW)
	IF    (PREFER_STATIC_LIBS)
		PREFER_STATIC_LIBS()
		FIND_LIBRARY(C_LIBRARY c)
		FIND_LIBRARY(MATH_LIBRARY m)
		#FIND_LIBRARY(PTHREAD_LIBRARY pthread)
		#FIND_LIBRARY(OMP_LIBRARY gomp) FIXME it's hidden in some subfolders
		UNPREFER_STATIC_LIBS()

		LIST(APPEND engineCommonLibraries ${C_LIBRARY} ${MATH_LIBRARY})
		#LIST(APPEND engineCommonLibraries ${PTHREAD_LIBRARY} ${OMP_LIBRARY})
	ENDIF (PREFER_STATIC_LIBS)

	# Needed for dynamically loading shared libraries (on some OS)
	LIST(APPEND engineCommonLibraries ${CMAKE_DL_LIBS})

	# Needed for backtrace* on some systems
	IF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		LIST(APPEND engineCommonLibraries execinfo)
	ENDIF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
ENDIF (UNIX AND NOT MINGW)

FIND_PACKAGE_STATIC(ZLIB REQUIRED)
LIST(APPEND engineCommonLibraries ${IL_LIBRARIES} ${JPEG_LIBRARY} ${PNG_LIBRARY} ${TIFF_LIBRARY} ${GIF_LIBRARY})
LIST(APPEND engineCommonLibraries ${Boost_REGEX_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SIGNALS_LIBRARY})
LIST(APPEND engineCommonLibraries ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY_WITH_RT} ${Boost_FILESYSTEM_LIBRARY})
LIST(APPEND engineCommonLibraries 7zip ${SPRING_MINIZIP_LIBRARY} ${ZLIB_LIBRARY})
LIST(APPEND engineCommonLibraries lua luasocket archives streflop assimp)
include_directories(${ZLIB_INCLUDE_DIR})

IF (WIN32)
	LIST(APPEND engineCommonLibraries ${WIN32_LIBRARIES})
ENDIF (WIN32)

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	#tcmalloc on freebsd is broken, default disable it (#4754)
	set(USE_TCMALLOC_DEFAULT FALSE)
else()
	set(USE_TCMALLOC_DEFAULT TRUE)
endif()

FIND_PACKAGE_STATIC(TCMalloc)
option(USE_TCMALLOC "use tcmalloc (part of google's perftools)" ${USE_TCMALLOC_DEFAULT})
if    (USE_TCMALLOC AND TCMALLOC_LIBRARY)
	MESSAGE(STATUS "Using tcmalloc")
	LIST(APPEND engineCommonLibraries ${TCMALLOC_LIBRARY})
endif (USE_TCMALLOC AND TCMALLOC_LIBRARY)


if(UNIX)
	FIND_PACKAGE_STATIC(Libunwind REQUIRED)
	LIST(APPEND engineCommonLibraries ${LIBUNWIND_LIBRARIES})
	if(LIBUNWIND_FOUND)
		message(STATUS "Found libunwind libraries at ${LIBUNWIND_LIBRARIES}")
	else()
		message(FATAL_ERROR "Couldn't find libunwind")
	endif()
endif()



### Assemble engine sources
Add_Subdirectory(Game)
Add_Subdirectory(Lua)
Add_Subdirectory(ExternalAI)
Add_Subdirectory(Rendering)
Add_Subdirectory(aGui)
Add_Subdirectory(Menu)
Add_Subdirectory(Map)
Add_Subdirectory(Net)
Add_Subdirectory(Sim)
#Add_Subdirectory(System) # this is already added in ../

MakeGlobalVar(engineSources
		${sources_engine_Game}
		${sources_engine_Net}
		${sources_engine_Lua}
		${sources_engine_Map}
		${sources_engine_Rendering}
		${sources_engine_Menu}
		${sources_engine_System}
		${sources_engine_ExternalAI}
	)

### Add headers for generated project files (e.g. Code::Blocks)
file(GLOB_RECURSE engineHeaders "*.h" "*.hpp" "*.inl")


Add_Subdirectory(builds)
