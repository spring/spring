# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

add_definitions(-DHEADLESS)
add_definitions(-DNO_SOUND)
add_definitions(-DBITMAP_NO_OPENGL)
remove_definitions(-DAVI_CAPTURING)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)


### Build the executable
add_executable(engine-headless ${engineSources} ${ENGINE_ICON})
target_link_libraries(engine-headless
	headlessStubs
	${SPRING_SIM_LIBRARIES}
	engineSystemNet
	${engineCommonLibraries}
	engineSim
	pr-downloader_static
	no-sound
)
target_include_directories(engine-headless PRIVATE
	${Spring_SOURCE_DIR}/rts
	${Spring_SOURCE_DIR}/rts/lib/lua/include
	${SDL2_INCLUDE_DIR}
	${ENGINE_SRC_ROOT_DIR}/lib/assimp/include
	${ENGINE_SRC_ROOT_DIR}/lib/asio/include
	${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include
	${SPRING_MINIZIP_INCLUDE_DIR}
	${LIBUNWIND_INCLUDE_DIRS}
)

if    (MINGW)
	# To enable console output/force a console window to open
	set_target_properties(engine-headless PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
endif ()


### Install the executable
install(TARGETS engine-headless DESTINATION ${BINDIR})

# Only build & install spring-headless executable & dependencies
# use cases:
# * make spring-headless
# * make install-spring-headless
create_engine_build_and_install_target(headless)
