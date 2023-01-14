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
set(engineIncludes "")
set(engineLibraries ${SDL2_LIBRARY})
if("${SDL2_VERSION_STRING}" VERSION_LESS "2")
	message(FATAL_ERROR "Found SDL v${SDL2_VERSION_STRING} while 2 is required!")
endif ()


set(OpenGL_GL_PREFERENCE LEGACY)
find_package_static(OpenGL REQUIRED)
find_package_static(GLEW 1.5.1 REQUIRED)
list(APPEND engineLibraries ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})

find_package_static(FONTCONFIG)
if    (FONTCONFIG_FOUND)
	list(APPEND engineIncludes ${FONTCONFIG_INCLUDE_DIR})
	if    (NOT EXISTS "${FONTCONFIG_INCLUDE_DIR}/fontconfig/fontconfig.h")
		message(FATAL_ERROR "FontConfig was found and will be used, but headers are missing!")
	endif ()
	list(APPEND engineLibraries ${FONTCONFIG_LIBRARIES})
	add_definitions(-DUSE_FONTCONFIG)
	if    (PREFER_STATIC_LIBS)
		prefer_static_libs()
		find_library(EXPAT_LIBRARY expat)
		unprefer_static_libs()
		list(APPEND engineLibraries ${EXPAT_LIBRARY})
	endif ()
endif ()

find_freetype_hack() # hack to find different named freetype.dll
find_package_static(Freetype REQUIRED)
if    (NOT EXISTS "${FREETYPE_INCLUDE_DIR_ft2build}/ft2build.h")
	message(FATAL_ERROR "FreeType 2 was found and will be used, but (at least some) headers are missing (ft2build.h)!")
endif ()
list(APPEND engineLibraries ${FREETYPE_LIBRARY})

if    (PREFER_STATIC_LIBS)
	# dependencies of FreeType
	find_package_static(BZip2 REQUIRED)
	list(APPEND engineLibraries ${BZIP2_LIBRARIES})
endif ()

if    (UNIX)
	find_package(X11 REQUIRED)
	list(APPEND engineLibraries ${X11_Xcursor_LIB} ${X11_X11_LIB} Threads::Threads)
endif ()

if (APPLE)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
endif ()



### Build the executable
add_executable(engine-opengl4 ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON} ${engineHeaders})
target_link_libraries(engine-opengl4
	${engineLibraries}
	${sound-impl}
	engineSystemNet
	${engineCommonLibraries}
	engineaGui
	${SPRING_SIM_LIBRARIES}
	engineSim
	pr-downloader_static
	engineMisc
)
target_include_directories(engine-opengl4 PRIVATE
	${Spring_SOURCE_DIR}/rts
	${Spring_SOURCE_DIR}/rts/lib/lua/include
	${ENGINE_SRC_ROOT_DIR}/lib/asio/include
	#${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include
	${ENGINE_SRC_ROOT_DIR}/lib/assimp/include
	${OPENAL_INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
	${SPRING_MINIZIP_INCLUDE_DIR}
)


### Install the executable
install(TARGETS engine-opengl4 DESTINATION ${BINDIR})

create_engine_build_and_install_target(opengl4)
