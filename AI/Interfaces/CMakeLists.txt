### AI Interfaces
## This section will only contain the C AI interface most likely,
## as all the other ones will need special libraries to compile and link,
## which not everyone will have installed.
#add_definitions(-DBUILDING_AI_INTERFACE -DSTREFLOP_X87)
add_definitions(-DBUILDING_AI_INTERFACE)

file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Interfaces/C/VERSION" C_AIINTERFACE_VERSION LIMIT_COUNT 1)
message("C AI Interface version is: ${C_AIINTERFACE_VERSION}")
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Interfaces/C cAIInterface)
list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/SharedLib)
if (UNIX)
	list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/SoLib)
else (UNIX)
	list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/DllLib)
endif (UNIX)
add_library("C-${C_AIINTERFACE_VERSION}" MODULE ${cAIInterface} ${aienv}) # ${errorhandler})
install (FILES "${CMAKE_SOURCE_DIR}/AI/Interfaces/C/InterfaceInfo.lua" DESTINATION "${DATADIR}/AI/Interfaces/data/C-${C_AIINTERFACE_VERSION}")

install (TARGETS "C-${C_AIINTERFACE_VERSION}" DESTINATION ${DATADIR}/AI/Interfaces/impls)

