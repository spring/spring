# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


### Assemble libraries
set(engineIncludes "")
set(engineLibraries ${SDL2_LINK_LIBRARIES})


set(OpenGL_GL_PREFERENCE LEGACY)

list(APPEND engineLibraries ${OPENGL_LINK_LIBRARIES} ${GLEW_LINK_LIBRARIES} ${IL_LINK_LIBRARIES})

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

pkg_check_modules(FREETYPE REQUIRED freetype2)
list(APPEND engineLibraries ${FREETYPE_LINK_LIBRARIES})

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
	${ENGINE_SRC_ROOT_DIR}/lib/assimp/include
	${OPENAL_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
	${SPRING_MINIZIP_INCLUDE_DIR}
)


### Install the executable
install(TARGETS engine-opengl4 DESTINATION ${BINDIR})

create_engine_build_and_install_target(opengl4)
