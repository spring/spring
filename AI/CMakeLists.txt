### AI
#
# Global variables set in this file:
# * AI_EXCLUDE_REGEX
# * AI_FIND_QUIETLY
# * DEPS_AI_ALL
#
# Functions and macros defined in this file:
# * AIMessage
#

add_definitions(-DBUILDING_AI)


### User-definable build options
set (AI_EXCLUDE_REGEX "XXXXXXXX" CACHE STRING "Which Skirmish AIs not to build (none by default, example: \"NTai|Null.*AI\", see also: AI_TYPES")
option(AI_FIND_QUIETLY  "Whether to find AI Interfaces and Skirmish AIs quietly" FALSE)

set(rts             "${CMAKE_SOURCE_DIR}/rts")

include_directories(
	${rts}
	${CMAKE_BINARY_DIR}/src-generated/engine
	${rts}/ExternalAI/Interface
	Wrappers
	)

# Set these for Interfaces and AIs with C sources
set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_CXX_FLAGS_PROFILE}")

remove_definitions(-DSTREFLOP_SSE)
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)

if    (MINGW)
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--kill-at -Wl,--add-stdcall-alias")
endif (MINGW)


# Assemble common additional native AI sources
#list(APPEND ai_common_SRC "${rts}/Game/GameVersion.cpp")
list(APPEND ai_common_SRC "${rts}/System/SafeCStrings.c")


# Set some Skirmish AI stuff here already,
# cause it is used by NullPythonAI in Interfaces/Python

# Export Paths needed for the AIs
set(SKIRMISH_AI_LIBS        "${AI_LIBS_DIR}/AI/Skirmish")
set(SKIRMISH_AI_DATA        "${AI_DATA_DIR}/AI/Skirmish")

# Display messages on configure, which AIs are found
set(SKIRMISHAI_FIND_QUIETLY ${AI_FIND_QUIETLY})

MakeGlobal(SKIRMISH_AI_LIBS SKIRMISH_AI_DATA SKIRMISHAI_FIND_QUIETLY)


# Print a message, but only if we are allowed to speak.
macro    (AIMessage type msg)
	if    (NOT AI_FIND_QUIETLY)
		message(${type} "${msg}")
	endif (NOT AI_FIND_QUIETLY)
endmacro (AIMessage type msg)


find_package(AWK)

# CUtils has to come first, because the Interfaces use it too
Add_Subdirectory(Wrappers/CUtils)
Add_Subdirectory(Interfaces)
Add_Subdirectory(Wrappers)
Add_Subdirectory(Skirmish)

set(DEPS_AI_ALL
		${DEPS_AI_INTERFACES}
		${DEPS_AI_WRAPPERS}
		${DEPS_AI_SKIRMISH}
	)
MakeGlobal(DEPS_AI_ALL)
