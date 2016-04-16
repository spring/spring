
Add_Subdirectory(unitsync)
Add_Subdirectory(DemoTool)

If    (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")
	MESSAGE(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/ is missing, please run\n git submodule init && git submodule update")
EndIf ()

# only add this if the submodule is present
find_package(CURL REQUIRED)
set(CMAKE_BUILD_TYPE "RELEASE")
set(CLEAR_COMPILER_FLAGS true)
Add_Subdirectory(pr-downloader)
# add install-pr-downloader target
Set(myInstallDeps pr-downloader)
Set(myInstallDirs "tools/pr-downloader")
CreateInstallTarget("pr-downloader" myInstallDeps myInstallDirs)

# This is not part of the official source package
if    (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
	Add_Subdirectory(dirchange)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
