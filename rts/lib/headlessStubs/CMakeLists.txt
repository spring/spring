
add_library(headlessStubs STATIC EXCLUDE_FROM_ALL
	glewstub.c
	glstub.c
	sdlstub.c
)

set_target_properties(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
target_include_directories(headlessStubs
PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
)
