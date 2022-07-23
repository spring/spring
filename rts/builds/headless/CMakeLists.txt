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
remove_definitions(-DAVI_CAPTURING)

set(OpenGL_GL_PREFERENCE LEGACY)
include_directories(${OPENAL_INCLUDE_DIR})

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# headlessstubs are our stubs that replace libGL, libGLU, libGLEW, libSDL (yes really!)
list(APPEND engineHeadlessLibraries meshoptimizer)
list(APPEND engineHeadlessLibraries headlessStubs)
list(APPEND engineHeadlessLibraries ${SPRING_SIM_LIBRARIES})
list(APPEND engineHeadlessLibraries engineSystemNet)
list(APPEND engineHeadlessLibraries ${engineCommonLibraries})
#list(APPEND engineHeadlessLibraries engineaGui)
list(APPEND engineHeadlessLibraries no-sound)
list(APPEND engineHeadlessLibraries engineSim)
list(APPEND engineHeadlessLibraries pr-downloader_static)

include_directories(${ENGINE_SRC_ROOT_DIR}/lib/assimp/include)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/asio/include)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include)


### Build the executable
add_executable(engine-headless ${engineSources} ${ENGINE_ICON})
target_link_libraries(engine-headless no-sound ${engineHeadlessLibraries} no-sound)

if    (MINGW)
	# To enable console output/force a console window to open
	set_target_properties(engine-headless PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
endif (MINGW)


### Install the executable
install(TARGETS engine-headless DESTINATION ${BINDIR})

# Only build & install spring-headless executable & dependencies
# use cases:
# * make spring-headless
# * make install-spring-headless
create_engine_build_and_install_target(headless)
