# Spring supplied CMake build file

# deactivate signaling-NANs for this library
remove_flag("-fsignaling-nans")

INCLUDE_DIRECTORIES(BEFORE ../streflop include)
FIND_PACKAGE_STATIC(GLEW 1.5.1 REQUIRED) #FIXME: LuaUser.cpp requires glew...
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})

SET(luaSources
		"src/lapi.cpp"
		"src/lauxlib.cpp"
		"src/lbaselib.cpp"
		"src/lcode.cpp"
		"src/ldblib.cpp"
		"src/ldebug.cpp"
		"src/ldo.cpp"
		"src/ldump.cpp"
		"src/lfunc.cpp"
		"src/lgc.cpp"
		"src/linit.cpp"
		"src/liolib.cpp"
		"src/llex.cpp"
		"src/lmathlib.cpp"
		"src/lmem.cpp"
		"src/loadlib.cpp"
		"src/lobject.cpp"
		"src/lopcodes.cpp"
		"src/loslib.cpp"
		"src/lparser.cpp"
		"src/lstate.cpp"
		"src/lstring.cpp"
		"src/lstrlib.cpp"
		"src/ltable.cpp"
		"src/ltablib.cpp"
		"src/ltm.cpp"
		"src/lundump.cpp"
		"src/lvm.cpp"
		"src/lzio.cpp"
		"src/print.cpp"
		"include/LuaUser.cpp"
	)

ADD_LIBRARY(lua STATIC EXCLUDE_FROM_ALL ${luaSources})

if (ENABLE_STREFLOP)
TARGET_LINK_LIBRARIES(lua streflop)
else ()
TARGET_LINK_LIBRARIES(lua)
endif ()

if (UNIX)
	SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS "-DLUA_USE_MKSTEMP ${PIC_FLAG}")
else (UNIX)
	SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
endif (UNIX)

