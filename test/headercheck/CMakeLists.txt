# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

option(HEADERCHECK "adds the target headercheck which compiles every header file, default disabled because it adds many targets" OFF)
if(HEADERCHECK)
include_directories(
		${Spring_SOURCE_DIR}/rts
		${Spring_SOURCE_DIR}/include
		${Spring_SOURCE_DIR}/rts/lib
		${Spring_SOURCE_DIR}/rts/lib/lua/include
		${SDL2_INCLUDE_DIR}
		${OPENAL_INCLUDE_DIR}
	)

file(GLOB_RECURSE headers
		${Spring_SOURCE_DIR}/tools/unitsync/*.h
		${Spring_SOURCE_DIR}/rts/ExternalAI/*.h
		${Spring_SOURCE_DIR}/rts/Game/*.h
		${Spring_SOURCE_DIR}/rts/Rendering/*.h
		${Spring_SOURCE_DIR}/rts/Map/*.h
		${Spring_SOURCE_DIR}/rts/aGui/*.h
		${Spring_SOURCE_DIR}/rts/Lua/*.h
		${Spring_SOURCE_DIR}/rts/Sim/*.h
		${Spring_SOURCE_DIR}/rts/Menu/*.h
		${Spring_SOURCE_DIR}/rts/System/*.h
	)

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
	list(APPEND headerchecklibs ${TEST_NAME})
endforeach( HEADER )

add_custom_target(headercheck DEPENDS ${headerchecklibs})

endif()
