SET(headlessStubsSources
		"glewstub.c"
		"glstub.c"
		"sdlstub.c"
	)



ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
target_include_directories(headlessStubs
	PRIVATE ${SDL2_INCLUDE_DIR}
)

