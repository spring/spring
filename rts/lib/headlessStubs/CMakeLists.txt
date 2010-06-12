SET(headlessStubsSources
		"glewstub"
		"glstub"
		"glustub"
		"glxextstub"
		"sdlstub"
		"sdlstub_cppbit"
	)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

