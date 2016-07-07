
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts)

ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(luasocket)
ADD_SUBDIRECTORY(7z)
ADD_SUBDIRECTORY(minizip)

Include(FindPkgConfig)
pkg_check_modules(STREFLOP streflop-sse)

if (STREFLOP_FOUND)
	message(STATUS ${STREFLOP_INCLUDE_DIRS})
	add_library(streflop streflop/streflopC.cpp)
	target_link_libraries(streflop ${STREFLOP_LIBRARIES})
	target_include_directories(streflop PUBLIC ${STREFLOP_INCLUDE_DIRS})
	set_target_properties(streflop PROPERTIES COMPILE_FLAGS ${PIC_FLAG})
else()
	include_directories(${CMAKE_CURRENT_SOURCE_DIR})
	ADD_SUBDIRECTORY(streflop)
endif()

ADD_SUBDIRECTORY(headlessStubs)

# minizip & unzip is the same, use minizip for assimp:
set(UNZIP_FOUND TRUE)
set(UNZIP_INCLUDE_DIRS ${SPRING_MINIZIP_INCLUDE_DIR}/minizip)
set(UNZIP_LIBRARIES ${SPRING_MINIZIP_LIBRARY})
set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
add_definitions(-DASSIMP_BUILD_NO_OWN_ZLIB)
ADD_SUBDIRECTORY(assimp)

if    (USE_LIBSQUISH)
	ADD_SUBDIRECTORY(rg-etc1)
	ADD_SUBDIRECTORY(squish)
endif (USE_LIBSQUISH)
