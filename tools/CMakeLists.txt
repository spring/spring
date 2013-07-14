
Add_Subdirectory(unitsync)
Add_Subdirectory(DemoTool)

If    (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")
	# only add this if the submodule is present
	find_package(CURL)
	If (CURL_FOUND)
		set(CMAKE_BUILD_TYPE "RELEASE")
		set(CLEAR_COMPILER_FLAGS true)
		Add_Subdirectory(pr-downloader)
		# add install-pr-downloader target
		Set(myInstallDeps pr-downloader)
		Set(myInstallDirs "tools/pr-downloader")
		CreateInstallTarget("pr-downloader" myInstallDeps myInstallDirs)
	Else(CURL_FOUND)
		MESSAGE(STATUS "pr-downloader will NOT be generated/installed; libcurl not found")
	EndIf(CURL_FOUND)
EndIf (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")

# This is not part of the official source package
if    (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
	Add_Subdirectory(dirchange)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
