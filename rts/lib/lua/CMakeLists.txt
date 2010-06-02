# Spring supplied CMake build file

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

