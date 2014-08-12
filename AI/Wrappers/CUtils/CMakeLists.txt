### CUtils AI Wrapper
#
# Global variables set in this file:
# * CUtils_SRC
#

set(myName         "CUtils")
set(mySourceDirRel ".")

# Assemble the sources
set(mySourceDir ${CMAKE_CURRENT_SOURCE_DIR}/${mySourceDirRel})
set(mySources
	"${mySourceDir}/SharedLibrary.c"
	"${mySourceDir}/SimpleLog.c"
	"${mySourceDir}/SimpleProfiler.cpp"
	"${mySourceDir}/SSkirmishAISpecifier.cpp"
	"${mySourceDir}/Util.c"
	"${mySourceDir}/TimeUtil.cpp"
	)
add_library(CUtils STATIC ${mySources})

# compile a second time with -DBUILDING_AI_INTERFACE
add_library(CUtilsAIInterface STATIC ${mySources})
set_target_properties(CUtilsAIInterface PROPERTIES COMPILE_DEFINITIONS "BUILDING_AI_INTERFACE")

AIMessage(STATUS "Found AI Wrapper: ${myName} (sources only wrapper)")
