# Spring supplied CMake build file

# deactivate signaling-NANs for this library
remove_flag("-fsignaling-nans")
find_package_static(GLEW 1.5.1 REQUIRED)

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

add_library(lua STATIC EXCLUDE_FROM_ALL ${luaSources})
target_include_directories(lua
PUBLIC
	include
	${GLEW_INCLUDE_DIR}
)

if (ENABLE_STREFLOP)
	target_link_libraries(lua streflop)
endif ()

if (UNIX)
	set_target_properties(lua PROPERTIES COMPILE_FLAGS "-DLUA_USE_MKSTEMP ${PIC_FLAG}")
else ()
	set_target_properties(lua PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
endif ()

