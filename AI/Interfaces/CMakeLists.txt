### AI Interfaces
#

add_definitions(-DBUILDING_AI_INTERFACE)

set(AI_INTERFACES_SRC        "${CMAKE_CURRENT_SOURCE_DIR}")
set(AI_INTERFACES_DATA       "${AIDIR}/AI/Interfaces")
set(AIINTERFACE_FIND_QUIETLY ${AI_FIND_QUIETLY})

# Add the sub-module only if its base source dir exists,
# and propagate the BUILD_${sourceDir}_AIINTERFACE value to ../,
# so it is accessible under ../Skirmish/.
# This is usefull when using git submodules for example.
macro    (AddAIInterface sourceDir)
	if    (EXISTS "${AI_INTERFACES_SRC}/${sourceDir}")
		Add_Subdirectory(${sourceDir})
	endif (EXISTS "${AI_INTERFACES_SRC}/${sourceDir}")
	set(BUILD_${sourceDir}_AIINTERFACE ${BUILD_${sourceDir}_AIINTERFACE} PARENT_SCOPE)
	set(${sourceDir}_AIINTERFACE_VERS  ${${sourceDir}_AIINTERFACE_VERS}  PARENT_SCOPE)
endmacro (AddAIInterface sourceDir)


# Print a message, but only if we are allowed to speak.
macro    (AIInterfaceMessage type msg)
	if    (NOT AIINTERFACE_FIND_QUIETLY)
		message(${type} "${msg}")
	endif (NOT AIINTERFACE_FIND_QUIETLY)
endmacro (AIInterfaceMessage type msg)


GetListOfSubModules(AI_INTERFACE_DIRS)
# Make the list available in the parent dir
set(AI_INTERFACE_DIRS ${AI_INTERFACE_DIRS} PARENT_SCOPE)

# Add each of these subdirs as an AI Interface submodule
foreach    (aiInterfaceDir ${AI_INTERFACE_DIRS})
	AddAIInterface(${aiInterfaceDir})
endforeach (aiInterfaceDir)


# ugly special treatment, but the only way known to work
set(CLASSPATH_Java_AIINTERFACE ${CLASSPATH_Java_AIINTERFACE} PARENT_SCOPE)
