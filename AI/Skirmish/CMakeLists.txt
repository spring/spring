### Skirmish AIs
#
# Functions and macros defined in this file:
# * SkirmishAIMessage
#

add_definitions(-DBUILDING_SKIRMISH_AI)

set(SKIRMISH_SRC            "${CMAKE_CURRENT_SOURCE_DIR}")

# Print a message, but only if we are allowed to speak.
macro    (SkirmishAIMessage type msg)
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "${msg}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)
endmacro (SkirmishAIMessage typemsg)


# Add all Skirmish AI submodules
GetListOfSubModules(SKIRMISH_AI_DIRS)
set(DEPS_AI_SKIRMISH "")
foreach    (skirmishAIDir ${SKIRMISH_AI_DIRS})
	Add_Subdirectory(${skirmishAIDir})
	if (TARGET ${skirmishAIDir})
		list(APPEND DEPS_AI_SKIRMISH ${skirmishAIDir})
	endif()
endforeach (skirmishAIDir)
MakeGlobal(DEPS_AI_SKIRMISH)
