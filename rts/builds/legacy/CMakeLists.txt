# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


### Assemble libraries
find_package(SDL2 REQUIRED)
set(engineIncludes ${SDL2_INCLUDE_DIR})
set(engineLibraries ${SDL2_LIBRARY})
if("${SDL2_VERSION_STRING}" VERSION_LESS "2")
	message(FATAL_ERROR "Found SDL v${SDL2_VERSION_STRING} while 2 is required!")
endif()


FIND_PACKAGE_STATIC(OpenGL REQUIRED)
FIND_PACKAGE_STATIC(GLEW 1.5.1 REQUIRED)
LIST(APPEND engineLibraries ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})
LIST(APPEND engineIncludes ${GLEW_INCLUDE_DIR})

FIND_PACKAGE_STATIC(FontConfig)
IF    (FONTCONFIG_FOUND)
	LIST(APPEND engineIncludes ${FONTCONFIG_INCLUDE_DIR})
	LIST(APPEND engineLibraries ${FONTCONFIG_LIBRARIES})
	ADD_DEFINITIONS(-DUSE_FONTCONFIG)
	IF    (PREFER_STATIC_LIBS)
		PREFER_STATIC_LIBS()
		find_library(EXPAT_LIBRARY expat)
		UNPREFER_STATIC_LIBS()
		list(APPEND engineLibraries ${EXPAT_LIBRARY})
	endif (PREFER_STATIC_LIBS)
endif (FONTCONFIG_FOUND)

FindFreetypeHack() # hack to find different named freetype.dll
find_package_static(Freetype REQUIRED)
foreach(f ${FREETYPE_INCLUDE_DIRS})
	list(APPEND engineIncludes ${f})
endforeach(f)
list(APPEND engineLibraries ${FREETYPE_LIBRARY})

if    (PREFER_STATIC_LIBS)
	# dependencies of FreeType
	find_package_static(BZip2 REQUIRED)
	list(APPEND engineLibraries ${BZIP2_LIBRARIES})
endif (PREFER_STATIC_LIBS)

if    (UNIX)
	find_package(X11 REQUIRED)
	list(APPEND engineLibraries ${X11_Xcursor_LIB} ${X11_X11_LIB})
endif (UNIX)

if (APPLE)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
endif (APPLE)

if    (USE_LIBSQUISH)
	list(APPEND engineLibraries squish rgetc1)
endif (USE_LIBSQUISH)

list(APPEND engineLibraries ${sound-impl})
list(APPEND engineLibraries engineSystemNet)
list(APPEND engineLibraries ${engineCommonLibraries})
list(APPEND engineLibraries engineaGui)
list(APPEND engineLibraries ${SPRING_SIM_LIBRARIES})
list(APPEND engineLibraries engineSim)
list(APPEND engineLibraries pr-downloader_static)

### Assemble external incude dirs
list(APPEND engineIncludes ${OPENAL_INCLUDE_DIR})
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/asio/include)
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include)

include_directories(${engineIncludes})

### Build the executable
add_executable(engine-legacy ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON} ${engineHeaders})
target_link_libraries(engine-legacy ${engineLibraries})


### Install the executable
install(TARGETS engine-legacy DESTINATION ${BINDIR})

CreateEngineBuildAndInstallTarget(legacy)
