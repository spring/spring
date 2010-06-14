SET(headlessStubsSources
		"glewstub"
		"glstub"
		"glustub"
		"glxextstub"
		"wglstub"
		"sdlstub"
		"sdlstub_cppbit"
	)

# We still need these header files,
# even if we are not going to link with SDL
FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
TARGET_LINK_LIBRARIES(headlessStubs ${Boost_THREAD_LIBRARY})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

