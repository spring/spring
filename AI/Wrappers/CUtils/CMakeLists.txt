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
SetGlobal(${myName}_SRC "${mySources}")

AIMessage(STATUS "Found AI Wrapper: ${myName} (sources only wrapper)")
