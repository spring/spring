# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


### Assemble libraries
FIND_PACKAGE(SDL2 REQUIRED)
SET(engineIncludes ${SDL2_INCLUDE_DIR})
SET(engineLibraries ${SDL2_LIBRARY})
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
		FIND_LIBRARY(EXPAT_LIBRARY expat)
		UNPREFER_STATIC_LIBS()
		LIST(APPEND engineLibraries ${EXPAT_LIBRARY})
	ENDIF (PREFER_STATIC_LIBS)
ENDIF (FONTCONFIG_FOUND)

FindFreetypeHack() # hack to find different named freetype.dll
FIND_PACKAGE_STATIC(Freetype REQUIRED)
foreach(f ${FREETYPE_INCLUDE_DIRS})
	LIST(APPEND engineIncludes ${f})
endforeach(f)
LIST(APPEND engineLibraries ${FREETYPE_LIBRARIES})

IF    (PREFER_STATIC_LIBS)
	# dependencies of FreeType
	FIND_PACKAGE_STATIC(BZip2 REQUIRED)
	LIST(APPEND engineLibraries ${BZIP2_LIBRARIES})
ENDIF (PREFER_STATIC_LIBS)

IF    (UNIX)
	FIND_PACKAGE_STATIC(X11 REQUIRED)
	LIST(APPEND engineLibraries ${X11_Xcursor_LIB} ${X11_X11_LIB})

	IF    (PREFER_STATIC_LIBS)
		# dependencies of X11/XCursor
		FIND_PACKAGE_STATIC(XCB REQUIRED)
		LIST(APPEND engineLibraries ${X11_Xrender_LIB} ${X11_Xfixes_LIB} ${XCB_LIBRARIES} ${X11_Xau_LIB} ${X11_Xdmcp_LIB})
	ENDIF (PREFER_STATIC_LIBS)
ENDIF (UNIX)

IF (APPLE)
	FIND_LIBRARY(COREFOUNDATION_LIBRARY Foundation)
	LIST(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
ENDIF (APPLE)

if    (USE_LIBSQUISH)
	LIST(APPEND engineLibraries squish rgetc1)
endif (USE_LIBSQUISH)

LIST(APPEND engineLibraries ${sound-impl})
LIST(APPEND engineLibraries engineSystemNet)
LIST(APPEND engineLibraries ${engineCommonLibraries})
LIST(APPEND engineLibraries engineaGui)
LIST(APPEND engineLibraries ${Boost_THREAD_LIBRARY})
LIST(APPEND engineLibraries ${SPRING_SIM_LIBRARIES})
LIST(APPEND engineLibraries engineSim)
LIST(APPEND engineLibraries pr-downloader_static)

### Assemble external incude dirs
LIST(APPEND engineIncludes ${OPENAL_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${engineIncludes})

### Build the executable
ADD_EXECUTABLE(engine-legacy ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON} ${engineHeaders})
TARGET_LINK_LIBRARIES(engine-legacy ${engineLibraries})


### Install the executable
INSTALL(TARGETS engine-legacy DESTINATION ${BINDIR})

CreateEngineBuildAndInstallTarget(legacy)

