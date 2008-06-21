### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
Project(Spring)

Set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/game)
Set(COMPLETE_BUILD true)

Add_Subdirectory(rts)
Add_Subdirectory(tools/DedicatedServer)
Add_Subdirectory(tools/unitsync)

if (UNIX)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else (UNIX)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif(UNIX)

install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game DESTINATION /opt/spring/ PATTERN ".svn" EXCLUDE)

