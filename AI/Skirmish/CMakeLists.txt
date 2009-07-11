### Skirmish AIs
#
# BUILD_C_AIINTERFACE and BUILD_Java_AIINTERFACE
# are set in ../Interfaces/CMakeLists.txt
#

add_definitions(-DBUILDING_SKIRMISH_AI)

set(SKIRMISH_SRC "${CMAKE_SOURCE_DIR}/AI/Skirmish")
set(SKIRMISH_DATA "${AIDIR}/AI/Skirmish")

# These seem to be needed when compiling with multiple threads.
# They are really defined in ../Interfaces/*/CMakeLists.txt.
# Using PARENT_SCOPE there seems not to work.
set(C_AIINTERFACE_TARGET    "C-AI-Interface")
set(Java_AIINTERFACE_TARGET "Java-AI-Interface")
if (BUILD_Java_AIINTERFACE)
	set(JAVA_FIND_QUIETLY TRUE)
	FIND_PACKAGE(JavaExtended) # Note: this is not the CMake built-in FindJava
endif (BUILD_Java_AIINTERFACE)


#set(SKIRMISHAI_FIND_QUIETLY True)


macro    (AddSkirmishAI sourceDir)
	# Add the sub-module only if its base source dir exists.
	# This is usefull when using git submodules for example.
	if    (EXISTS "${SKIRMISH_SRC}/${sourceDir}")
		Add_Subdirectory(${sourceDir})
	endif (EXISTS "${SKIRMISH_SRC}/${sourceDir}")
endmacro (AddSkirmishAI sourceDir)


GetListOfSubModules(SKIRMISH_AI_DIRS)
# Make the list available in the parent dir
set(SKIRMISH_AI_DIRS ${SKIRMISH_AI_DIRS} PARENT_SCOPE)

# Add each of these subdirs as a Skirmish AI submodule
foreach    (skirmishAIDir ${SKIRMISH_AI_DIRS})
	AddSkirmishAI(${skirmishAIDir})
endforeach (skirmishAIDir)
