
### AI Interfaces
## This section will only contain the C AI interface most likely,
## as all the other ones will need special libraries to compile and link,
## which not everyone will have installed.
#add_definitions(-DBUILDING_AI_INTERFACE -DSTREFLOP_X87)
add_definitions(-DBUILDING_AI_INTERFACE)

aux_source_directory(C cAIInterface)
list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/SharedLib)
if (UNIX)
	list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/SoLib)
else (UNIX)
	list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/DllLib)
endif (UNIX)
add_library(C-0.1 SHARED ${cAIInterface} ${aienv} ${luaparsingenv})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Interfaces/C/InterfaceInfo.lua DESTINATION ${DATADIR}/AI/Interfaces/data/C-0.1)

install (TARGETS C-0.1 DESTINATION ${DATADIR}/AI/Interfaces/impls)

