### Skirmish AIs
#
# Functions and macros defined in this file:
# * skirmish_ai_message
#

add_definitions(-DBUILDING_SKIRMISH_AI)

set(SKIRMISH_SRC            "${CMAKE_CURRENT_SOURCE_DIR}")

# Print a message, but only if we are allowed to speak.
macro    (skirmish_ai_message type msg)
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "${msg}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)
endmacro (skirmish_ai_message typemsg)


# Add all Skirmish AI submodules
get_list_of_submodules(SKIRMISH_AI_DIRS)
set(DEPS_AI_SKIRMISH "")
foreach    (skirmishAIDir ${SKIRMISH_AI_DIRS})
	add_subdirectory(${skirmishAIDir})
	if (TARGET ${skirmishAIDir})
		list(APPEND DEPS_AI_SKIRMISH ${skirmishAIDir})
	endif()
endforeach (skirmishAIDir)
make_global(DEPS_AI_SKIRMISH)
