# Spring supplied CMake build file

set(luaSocketSources
	src/auxiliar.cpp
	src/buffer.cpp
	src/except.cpp
	src/inet.cpp
	src/io.cpp
	src/luasocket.cpp
	src/mime.cpp
	src/options.cpp
	src/select.cpp
	src/tcp.cpp
	src/timeout.cpp
	src/udp.cpp
	src/restrictions.cpp
)

if(WIN32)
	list(APPEND luaSocketSources
		src/wsocket.cpp
	)
else()
	list(APPEND luaSocketSources
		src/usocket.cpp
		src/unix.cpp
	)
endif()

add_library(luasocket STATIC EXCLUDE_FROM_ALL ${luaSocketSources})
target_include_directories(luasocket
	PRIVATE
		../lua/include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
	DESTINATION ${DATADIR}
	FILES_MATCHING PATTERN "*.lua"
)

