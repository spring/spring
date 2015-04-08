SET(headlessStubsSources
		"glewstub"
		"glstub"
		"glustub"
		"sdlstub"
		"sdlstub_cppbit"
	)

# Use a direct copy of the GL and SDL headers,
# as we support to compile spring-headless without GL & SDL2 installed
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/include/SDL2)

ADD_LIBRARY(headlessStubs STATIC EXCLUDE_FROM_ALL ${headlessStubsSources})
TARGET_LINK_LIBRARIES(headlessStubs ${Boost_THREAD_LIBRARY} ${Boost_CHRONO_LIBRARY_WITH_RT})
SET_TARGET_PROPERTIES(headlessStubs PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

