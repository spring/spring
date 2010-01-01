INCLUDE_DIRECTORIES(../../rts/ ../../rts/System ../../rts/Game ../../rts/Sim/Misc)

SET(build_files
	../../rts/Game/GameVersion
	../../rts/Game/PlayerStatistics
	../../rts/Sim/Misc/TeamStatistics
	../../rts/System/Net/RawPacket
	../../rts/System/DemoReader
	../../rts/System/Demo)
IF (WIN32)
	SET(build_files ${build_files}
		../../rts/System/Platform/Win/WinVersion)
ENDIF (WIN32)

ADD_EXECUTABLE(demotool EXCLUDE_FROM_ALL DemoTool ${build_files})
IF (MINGW)
	# To enable console output/force a console window to open
	SET_TARGET_PROPERTIES(demotool PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
ENDIF (MINGW)
TARGET_LINK_LIBRARIES(demotool  ${Boost_PROGRAM_OPTIONS_LIBRARY})


