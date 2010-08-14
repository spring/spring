### AI
#
# Global variables set in this file:
# * AI_EXCLUDE_REGEX
# * AI_FIND_QUIETLY
# * DEPS_AI_ALL
#

add_definitions(-DBUILDING_AI)


### User-definable build options
set (AI_EXCLUDE_REGEX "XXXXXXXX" CACHE STRING "Which Skirmish AIs not to build (none by default, example: \"NTai|Null.*AI\", see also: AI_TYPES")
set (AI_FIND_QUIETLY  FALSE      CACHE BOOL   "Whether to find AI Interfaces and Skirmish AIs quietly")

set(rts             "${CMAKE_SOURCE_DIR}/rts")

include_directories(
	${rts}
	${rts}/System
	${rts}/ExternalAI/Interface
	Wrappers
	)

# Set these for Interfaces and AIs with C sources
set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2}")
set(CMAKE_C_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3}")
set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_CXX_FLAGS_PROFILE}")

remove_definitions(-DSTREFLOP_SSE)
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)

if    (WIN32)
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--kill-at -Wl,--add-stdcall-alias")
endif (WIN32)


# Assemble common additional native AI sources
list(APPEND ai_common_SRC ${rts}/Game/GameVersion)

# Assemble the creg sources
aux_source_directory("${rts}/System/creg" creg_SRC)


# Set some Skirmish AI stuff here already,
# cause it is used by NullPythonAI in Interfaces/Python

# Export Paths needed for the AIs
set(SKIRMISH_AI_LIBS        "${AI_LIBS_DIR}/AI/Skirmish")
set(SKIRMISH_AI_DATA        "${AI_DATA_DIR}/AI/Skirmish")

# Display messages on configure, which AIs are found
set(SKIRMISHAI_FIND_QUIETLY ${AI_FIND_QUIETLY})

MakeGlobal(SKIRMISH_AI_LIBS SKIRMISH_AI_DATA SKIRMISHAI_FIND_QUIETLY)


# Wrappers have to come first cause the Interfaces use the CUtils Wrapper too
Add_Subdirectory(Wrappers)
Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)

set(DEPS_AI_ALL
		${DEPS_AI_INTERFACES}
		${DEPS_AI_WRAPPERS}
		${DEPS_AI_SKIRMISH}
	)
MakeGlobal(DEPS_AI_ALL)
