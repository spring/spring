SET(headlessStubsSources
		"glewstub"
		"glstub"
		"glustub"
		"glxextstub"
		"sdlstub"
		"sdlstub_cppbit"
	)
IF    (WIN32)
	SET(headlessStubsSources ${headlessStubsSources}
			"wglstub"
		)
ENDIF (WIN32)

IF    (MINGW)
	# We still need these header files,
	# even if we are not going to link with SDL.
	# We have them available anyway (mingwlibs).
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
ELSE  (MINGW)
	# Use a direct copy of the GL and SDL headers,
	# as these may not be available on headless systems.
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/SDL)
ENDIF (MINGW)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
TARGET_LINK_LIBRARIES(headlessStubs ${Boost_THREAD_LIBRARY})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

