
Add_Subdirectory(unitsync)
Add_Subdirectory(DemoTool)

If    (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")
	# only add this if the submodule is present
	find_package(CURL)
	If (CURL_FOUND)
		Add_Subdirectory(pr-downloader)
	Else(CURL_FOUND)
		MESSAGE(WARNING "libcurl not found, pr-downloader disabled")
	EndIf(CURL_FOUND)
EndIf (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pr-downloader/CMakeLists.txt")

# This is not part of the official source package
if    (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
	Add_Subdirectory(dirchange)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
