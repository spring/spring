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
	set(JAVA_FIND_QUIETLY ${AIINTERFACE_FIND_QUIETLY})
	FIND_PACKAGE(JavaExtended) # Note: this is not the CMake builtin FindJava
endif (BUILD_Java_AIINTERFACE)


#set(SKIRMISHAI_FIND_QUIETLY True)


macro    (AddSkirmishAI sourceDir)
	# Add the sub-module only if its base source dir exists.
	# This is usefull when using git submodules for example.
	if    (EXISTS "${SKIRMISH_SRC}/${sourceDir}")
		Add_Subdirectory(${sourceDir})
	endif (EXISTS "${SKIRMISH_SRC}/${sourceDir}")
endmacro (AddSkirmishAI sourceDir)

AddSkirmishAI(KAIK)
AddSkirmishAI(RAI)
AddSkirmishAI(AAI)
AddSkirmishAI(NTai)

AddSkirmishAI(NullLegacyCppAI)
AddSkirmishAI(NullAI)

AddSkirmishAI(NullJavaAI)
AddSkirmishAI(NullOOJavaAI)
