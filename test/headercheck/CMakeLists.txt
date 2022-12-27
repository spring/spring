# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

option(HEADERCHECK "adds the target headercheck which compiles every header file, default disabled because it adds many targets" OFF)
if(HEADERCHECK)

file(GLOB_RECURSE headers
	${Spring_SOURCE_DIR}/**/*.h
)
if(UNIX)
	list(FILTER headers EXCLUDE REGEX "^${Spring_SOURCE_DIR}/rts/System/Platform/Win.*")
endif()
list(FILTER headers EXCLUDE REGEX "^${Spring_SOURCE_DIR}/rts/System/creg/.*")
list(FILTER headers EXCLUDE REGEX "^${Spring_SOURCE_DIR}/rts/lib/.*")
list(FILTER headers EXCLUDE REGEX "^${Spring_SOURCE_DIR}/AI/.*")
list(FILTER headers EXCLUDE REGEX "^${Spring_SOURCE_DIR}/tools/.*")

set(headerchecklibs "")

foreach( HEADER ${headers} )

	#pseudo unique name
	string(REPLACE "${Spring_SOURCE_DIR}" "" PLIST "${HEADER}")
	string(REPLACE "/" ";" PLIST "${PLIST}")
	set(TEST_NAME "headercheck_")

	foreach(name ${PLIST})
		set(TEST_NAME "${TEST_NAME}_${name}")
	endforeach()

	set(TEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/src-generated/${TEST_NAME}.cpp)

	# create test file
	configure_file( headercheck.cpp.in ${TEST_FILE} )
	# add library
	add_library( ${TEST_NAME} EXCLUDE_FROM_ALL ${TEST_FILE} )
	target_include_directories(${TEST_NAME} PRIVATE
		${Spring_SOURCE_DIR}/rts
		${Spring_SOURCE_DIR}/rts/lib
		${Spring_SOURCE_DIR}/rts/lib/lua/include
		${Spring_SOURCE_DIR}/rts/lib/slimsig/include
		${Spring_SOURCE_DIR}/rts/lib/assimp/include
		${Spring_SOURCE_DIR}/rts/lib/asio/include
		${SDL2_INCLUDE_DIR}
		${OPENAL_INCLUDE_DIR}
	)

	list(APPEND headerchecklibs ${TEST_NAME})
endforeach()

add_custom_target(headercheck DEPENDS ${headerchecklibs})

endif ()
