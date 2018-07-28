# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

set(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")


option(TRACE_SYNC "Enable sync tracker" FALSE)
if (TRACE_SYNC)
	add_definitions(-DTRACE_SYNC)
endif (TRACE_SYNC)

option(SYNCDEBUG "Enable sync debugger (needs SYNCCHECK=true)" FALSE)
if (SYNCDEBUG)
	add_definitions(-DSYNCDEBUG)
	if     (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
		message(WARNING "It is recommended to set CMAKE_BUILD_TYPE to DEBUG for a SYNCDEBUG build")
	endif  ()
	if     (NOT SYNCCHECK)
		message(FATAL_ERROR "You need SYNCCHECK=TRUE for a SYNCDEBUG build")
	endif  ()
	if     (NOT TRACE_SYNC)
		message(WARNING "It is recommended to use TRACE_SYNC=TRUE for a SYNCDEBUG build")
	endif  ()
endif (SYNCDEBUG)

option(DEBUG_GLSTATE "enable GL_STATE_CHECKER" FALSE)
if(DEBUG_GLSTATE)
	add_definitions(-DDEBUG_GLSTATE)
endif()

### give error when not found
find_package_static(DevIL REQUIRED)

### Assemble common incude dirs
include_directories(BEFORE lib/lua/include)
include_directories(${CMAKE_SOURCE_DIR}/include/AL)
include_directories(${SPRING_MINIZIP_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${IL_INCLUDE_DIR})

### Assemble common libraries
add_subdirectory(System/Sound)
if    (NO_SOUND)
	add_definitions(-DNO_SOUND)
endif (NO_SOUND)

### Find include directories and add platform specific libraries
if    (UNIX AND NOT MINGW)
	if    (PREFER_STATIC_LIBS)
		PREFER_STATIC_LIBS()
		find_library(C_LIBRARY c)
		find_library(MATH_LIBRARY m)
		#FIND_LIBRARY(PTHREAD_LIBRARY pthread)
		#FIND_LIBRARY(OMP_LIBRARY gomp) FIXME it's hidden in some subfolders
		UNPREFER_STATIC_LIBS()

		list(APPEND engineCommonLibraries ${C_LIBRARY} ${MATH_LIBRARY})
		#list(APPEND engineCommonLibraries ${PTHREAD_LIBRARY} ${OMP_LIBRARY})
	endif (PREFER_STATIC_LIBS)

	# Needed for dynamically loading shared libraries (on some OS)
	list(APPEND engineCommonLibraries ${CMAKE_DL_LIBS})

	# Needed for backtrace* on some systems
	if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		list(APPEND engineCommonLibraries execinfo)
	endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
endif (UNIX AND NOT MINGW)

find_package_static(ZLIB REQUIRED)
list(APPEND engineCommonLibraries ${IL_LIBRARIES} ${JPEG_LIBRARY} ${PNG_LIBRARY} ${TIFF_LIBRARY} ${GIF_LIBRARY})
list(APPEND engineCommonLibraries 7zip ${SPRING_MINIZIP_LIBRARY} ${ZLIB_LIBRARY})
list(APPEND engineCommonLibraries lua luasocket archives assimp gflags)
if (ENABLE_STREFLOP)
	list(APPEND engineCommonLibraries streflop)
endif ()

include_directories(${ZLIB_INCLUDE_DIR})

IF (WIN32)
	list(APPEND engineCommonLibraries ${WIN32_LIBRARIES})
ENDIF (WIN32)

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	#tcmalloc on freebsd is broken, default disable it (#4754)
	set(USE_TCMALLOC_DEFAULT FALSE)
else()
	set(USE_TCMALLOC_DEFAULT TRUE)
endif()

find_package_static(TCMalloc)
option(USE_TCMALLOC "use tcmalloc (part of google's perftools)" ${USE_TCMALLOC_DEFAULT})
if    (USE_TCMALLOC AND TCMALLOC_LIBRARY)
	message(STATUS "Using tcmalloc")
	list(APPEND engineCommonLibraries ${TCMALLOC_LIBRARY})
endif (USE_TCMALLOC AND TCMALLOC_LIBRARY)


if(UNIX)
	find_package_static(Libunwind REQUIRED)
	list(APPEND engineCommonLibraries ${LIBUNWIND_LIBRARIES})
	if(LIBUNWIND_FOUND)
		message(STATUS "Found libunwind libraries at ${LIBUNWIND_LIBRARIES}")
	else()
		message(FATAL_ERROR "Couldn't find libunwind")
	endif()
endif()



### Assemble engine sources
add_subdirectory(Game)
add_subdirectory(Lua)
add_subdirectory(ExternalAI)
add_subdirectory(Rendering)
add_subdirectory(aGui)
add_subdirectory(Menu)
add_subdirectory(Map)
add_subdirectory(Net)
add_subdirectory(Sim)
#add_subdirectory(System) # this is already added in ../

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/assimp/include) #FIXME: hack for rts/Rendering/Models/IModelParser.cpp

### Add headers for generated project files (e.g. Code::Blocks)
file(GLOB_RECURSE engineHeaders "*.h" "*.hpp" "*.inl")


add_subdirectory(builds)
