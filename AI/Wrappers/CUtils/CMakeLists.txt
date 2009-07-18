### CUtils AI Wrapper
#
# Global variables set in this file:
# * CUtils_SRC
#

set(myName            "CUtils")

# Assemble the sources
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} mySources)
SetGlobal(${myName}_SRC ${mySources})

AIWrapperMessage(STATUS "Found AI Wrapper: ${myName} (sources only wrapper)")
