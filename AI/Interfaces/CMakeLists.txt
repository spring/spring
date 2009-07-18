### AI Interfaces
#
# Functions and macros defined in this file:
# * AIInterfaceMessage
#

add_definitions(-DBUILDING_AI_INTERFACE)

set(AI_INTERFACES_SRC        "${CMAKE_CURRENT_SOURCE_DIR}")
set(AI_INTERFACES_DATA       "${AIDIR}/AI/Interfaces")
set(AIINTERFACE_FIND_QUIETLY ${AI_FIND_QUIETLY})


# Print a message, but only if we are allowed to speak.
macro    (AIInterfaceMessage type msg)
	if    (NOT AIINTERFACE_FIND_QUIETLY)
		message(${type} "${msg}")
	endif (NOT AIINTERFACE_FIND_QUIETLY)
endmacro (AIInterfaceMessage type msg)


# Add all AI Interface submodules
GetListOfSubModules(AI_INTERFACE_DIRS)
foreach    (aiInterfaceDir ${AI_INTERFACE_DIRS})
	Add_Subdirectory(${aiInterfaceDir})
endforeach (aiInterfaceDir)
