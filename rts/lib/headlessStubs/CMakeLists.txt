INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/rts/System)

INCLUDE_DIRECTORIES(BEFORE ${SDL_INCLUDE_DIR})
SET(headlessStubsFiles
		"glewstub.c"
		"glstub.c"
		"glustub.c"
		"sdlstub.c"
		"sdlstub_cppbit.cpp"
	)
ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsFiles})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

