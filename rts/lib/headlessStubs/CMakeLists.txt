SET(headlessStubsSources
		"glewstub"
		"glstub"
		"glustub"
		"glxextstub"
		"wglstub"
		"sdlstub"
		"sdlstub_cppbit"
	)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

