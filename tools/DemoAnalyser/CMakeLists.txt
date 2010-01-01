#ADD_DEFINITIONS(-DDEDICATED -DNOT_USING_CREG)

INCLUDE_DIRECTORIES(../../rts/ ../../rts/System)

SET(build_files
	../../rts/Game/GameVersion
	../../rts/System/Net/RawPacket
	../../rts/System/DemoReader
	../../rts/System/Demo)
IF (WIN32)
	SET(build_files ${build_files}
		../../rts/System/Platform/Win/WinVersion)
ENDIF (WIN32)

ADD_EXECUTABLE(demoanalyser EXCLUDE_FROM_ALL main ${build_files})
IF (MINGW)
	# To enable console output/force a console window to open
	SET_TARGET_PROPERTIES(demoanalyser PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
ENDIF (MINGW)
TARGET_LINK_LIBRARIES(demoanalyser)

