### AI Wrappers
#
# Functions and macros defined in this file:
# * ai_wrapper_message
#

#add_definitions(-DBUILDING_AI_WRAPPER)

set(AI_WRAPPER_SRC         "${CMAKE_CURRENT_SOURCE_DIR}")
set(AIWRAPPER_FIND_QUIETLY ${AI_FIND_QUIETLY})


# Print a message, but only if we are allowed to speak.
macro    (ai_wrapper_message type msg)
	if    (NOT AIWRAPPER_FIND_QUIETLY)
		message(${type} "${msg}")
	endif (NOT AIWRAPPER_FIND_QUIETLY)
endmacro (ai_wrapper_message type msg)


# Configure all AI Wrapper submodules that need processing
get_list_of_submodules(AI_WRAPPER_DIRS)
set(DEPS_AI_WRAPPERS "")
foreach    (aiWrapperDir ${AI_WRAPPER_DIRS})
	# exclude CUtils here, as it was already included before
	if    (NOT "${aiWrapperDir}" STREQUAL "CUtils")
		add_subdirectory(${aiWrapperDir})
		if(TARGET ${aiWrapperDir}-AIWrapper)
			list(APPEND DEPS_AI_WRAPPERS ${aiWrapperDir}-AIWrapper)
		endif()
	endif (NOT "${aiWrapperDir}" STREQUAL "CUtils")
endforeach (aiWrapperDir)
make_global(DEPS_AI_WRAPPERS)
