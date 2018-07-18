### AI Interfaces
#
# Functions and macros defined in this file:
# * AIInterfaceMessage
#

add_definitions(-DBUILDING_AI_INTERFACE)

set(AI_INTERFACES_SRC        "${CMAKE_CURRENT_SOURCE_DIR}")
set(AI_INTERFACES_LIBS       "${AI_LIBS_DIR}/AI/Interfaces")
set(AI_INTERFACES_DATA       "${AI_DATA_DIR}/AI/Interfaces")
set(AIINTERFACE_FIND_QUIETLY ${AI_FIND_QUIETLY})


# Print a message, but only if we are allowed to speak.
macro    (AIInterfaceMessage type msg)
	if    (NOT AIINTERFACE_FIND_QUIETLY)
		message(${type} "${msg}")
	endif (NOT AIINTERFACE_FIND_QUIETLY)
endmacro (AIInterfaceMessage type msg)


# Add all AI Interface submodules
GetListOfSubModules(AI_INTERFACE_DIRS)
set(DEPS_AI_INTERFACES "")
foreach    (aiInterfaceDir ${AI_INTERFACE_DIRS})
	if     (NOT "${aiInterfaceDir}" STREQUAL "Python")
	Add_Subdirectory(${aiInterfaceDir})
	if(TARGET ${aiInterfaceDir}-AIInterface)
		list(APPEND DEPS_AI_INTERFACES ${aiInterfaceDir}-AIInterface)
	endif()
	elseif (NOT "${aiInterfaceDir}" STREQUAL "Python")
		AIInterfaceMessage(STATUS "Python AI Interface is not yet compatible with pureint changes -> temporarily disabled")
	endif  (NOT "${aiInterfaceDir}" STREQUAL "Python")
endforeach (aiInterfaceDir)
MakeGlobal(DEPS_AI_INTERFACES)
