
add_subdirectory(unitsync)
add_subdirectory(DemoTool)
add_subdirectory(mapcompile)

if    (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")
	message(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/ is missing, please run\n git submodule init && git submodule update")
endif ()

set(CMAKE_BUILD_TYPE "RELEASE")
set(CLEAR_COMPILER_FLAGS true)
set(PRD_TESTS OFF CACHE BOOL "" FORCE)
set(PRD_BINDIR ${BINDIR} CACHE PATH "" FORCE)
set(PRD_LIBDIR ${LIBDIR} CACHE PATH "" FORCE)
add_subdirectory(pr-downloader)
# add install-pr-downloader target
set(myInstallDeps pr-downloader)
set(myInstallDirs "tools/pr-downloader")
create_install_target("pr-downloader" myInstallDeps myInstallDirs)

# This is not part of the official source package
if    (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
	add_subdirectory(dirchange)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
