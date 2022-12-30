add_library(engineLoadsave STATIC
	CregLoadSaveHandler.cpp
	Demo.cpp
	DemoReader.cpp
	DemoRecorder.cpp
	LoadSaveHandler.cpp
	LuaLoadSaveHandler.cpp
)
target_include_directories(engineLoadsave
	PRIVATE
		${Spring_SOURCE_DIR}/rts
		${SDL2_INCLUDE_DIR}
		${Spring_SOURCE_DIR}/rts/lib/lua/include
	PUBLIC
		${SPRING_MINIZIP_INCLUDE_DIR}
)
