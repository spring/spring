
if    (WIN32)
	Add_Subdirectory(ArchiveMover)
endif (WIN32)

Add_Subdirectory(unitsync)
Add_Subdirectory(DemoTool)

# This is not part of the official source package
if    (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
	Add_Subdirectory(dirchange)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dirchange)
