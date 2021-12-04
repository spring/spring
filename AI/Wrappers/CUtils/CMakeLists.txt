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
	"${mySourceDir}/Util.c"
	"${mySourceDir}/TimeUtil.cpp"
	)
add_library(CUtils STATIC ${mySources})
target_link_libraries(CUtils ${CMAKE_DL_LIBS})

ai_message(STATUS "Found AI Wrapper: ${myName} (sources only wrapper)")
