# Spring supplied CMake build file

SET(LUAJIT_FIND_REQUIRED FALSE)
SET(LUAJIT_FIND_QUIETLY FALSE)
Find_Package(LuaJIT)


If     (LUAJIT_FOUND)
	Message(STATUS "Using LuaJIT library: ${LUAJIT_LIBRARY}")

	SetGlobal(SPRING_LUAJIT_INCLUDE_DIR "${LUAJIT_INCLUDE_DIR}")
	SetGlobal(SPRING_LUAJIT_LIBRARY "${LUAJIT_LIBRARY}")
Else   (LUAJIT_FOUND)
	Message(STATUS "Using LuaJIT library: inbuild Lua (non-JIT)")

	RemoveFlag("-fsignaling-nans")

	INCLUDE_DIRECTORIES(BEFORE ../streflop include)

	SET(luaSources
			"src/lapi"
			"src/lauxlib"
			"src/lbaselib"
			"src/lcode"
			"src/ldblib"
			"src/ldebug"
			"src/ldo"
			"src/ldump"
			"src/lfunc"
			"src/lgc"
			"src/linit"
			"src/liolib"
			"src/llex"
			"src/lmathlib"
			"src/lmem"
			"src/loadlib"
			"src/lobject"
			"src/lopcodes"
			"src/loslib"
			"src/lparser"
			"src/lstate"
			"src/lstring"
			"src/lstrlib"
			"src/ltable"
			"src/ltablib"
			"src/ltm"
			"src/lundump"
			"src/lvm"
			"src/lzio"
			"src/print"
		)

	ADD_LIBRARY(lua STATIC EXCLUDE_FROM_ALL ${luaSources})
	TARGET_LINK_LIBRARIES(lua streflop)
	if (UNIX)
		SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS "-DLUA_USE_MKSTEMP ${PIC_FLAG}")
	else (UNIX)
		SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
	endif (UNIX)

	SetGlobal(SPRING_LUAJIT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/rts/lib/lua/include")
	SetGlobal(SPRING_LUAJIT_LIBRARY lua)
EndIf  (LUAJIT_FOUND)
