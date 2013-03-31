# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


### Assemble libraries
FIND_PACKAGE(SDL REQUIRED)
SET(engineIncludes ${SDL_INCLUDE_DIR})
SET(engineLibraries ${SDL_LIBRARY})


FIND_PACKAGE_STATIC(OpenGL REQUIRED)
FIND_PACKAGE_STATIC(GLU REQUIRED)
FIND_PACKAGE_STATIC(GLEW 1.5.1 REQUIRED)
LIST(APPEND engineLibraries ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})
if   (MSVC)
	LIST(APPEND engineIncludes ${GLEW_INCLUDE_DIR})
endif(MSVC)

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

LIST(APPEND engineLibraries ${engineCommonLibraries})

LIST(APPEND engineLibraries ${Boost_THREAD_LIBRARY})
if (WIN32 AND Boost_CHRONO_FOUND)
	# in newer boost versions the thread library depends on it
	LIST(APPEND engineLibraries ${Boost_CHRONO_LIBRARY})
endif (WIN32 AND Boost_CHRONO_FOUND)

### Assemble external incude dirs
LIST(APPEND engineIncludes ${OPENAL_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${engineIncludes})

if   (MSVC)
	# set vmg flag to work around small pointer-to-member bug in spring
	# and enable multi-processor compiling
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg /MP")
endif(MSVC)

### Add icon and manifest to exe using windres
IF    (WIN32)
	IF   (MINGW)
		SET(ENGINE_ICON_DIR "${ENGINE_SRC_ROOT_DIR}")
		SET(ENGINE_ICON_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
		SET(ENGINE_ICON_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
		CreateResourceCompileCommand(ENGINE_ICON "${ENGINE_ICON_DIR}" "${ENGINE_ICON_RES}" "${ENGINE_ICON_OBJ}")
	ENDIF(MINGW)
ELSE  (WIN32)
	SET(ENGINE_ICON "")
ENDIF (WIN32)

### allow engineLibraries to be used in multithreaded, too
MakeGlobal(engineLibraries)
MakeGlobal(engineIncludes)

### Build the executable
ADD_EXECUTABLE(engine-default ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON} ${engineHeaders})
TARGET_LINK_LIBRARIES(engine-default ${engineLibraries})
SET_TARGET_PROPERTIES(engine-default PROPERTIES OUTPUT_NAME "spring-singlethreaded")


### Install the executable
INSTALL(TARGETS engine-default DESTINATION ${BINDIR})

#IF    (MINGW)
#	SET_TARGET_PROPERTIES(engine-default PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
#	install (FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
#ENDIF (MINGW)

# Only build & install spring (default) executable & dependencies
# use cases:
# * make spring-default
# * make install-spring-default
CreateEngineBuildAndInstallTarget(default)

# Create shortcuts
# * make spring
# * make install-spring
add_custom_target(spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
add_dependencies(spring spring-default) # This also works for custom targets

add_custom_target(install-spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
add_dependencies(install-spring install-spring-default) # This also works for custom targets
