
### Skirmish AIs
#remove_definitions(-DBUILDING_AI_INTERFACE -DSTREFLOP_X87 -DSTREFLOP_SSE -DSTREFLOP_SOFT)
add_definitions(-DBUILDING_AI)

## KAI
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/KAI-0.2 kai02)
#add_library(KAI-0.2 MODULE ${kai02} ${legacycppaienv} ${creg})

## KAIK
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13 kaik013)
add_library(KAIK-0.13 MODULE ${kaik013} ${legacycppaienv} ${creg})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/KAIK-0.13)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/KAIK-0.13)

## RAI
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553 rai)
add_library(RAI-0.553 MODULE ${rai} ${legacycppaienv} ${creg} RAI-0.553/KrogsMetalClass/MetalMap)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/RAI-0.553)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/RAI-0.553)

## AAI
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI aai)
#add_library(AAI MODULE ${aai} ${legacycppaienv})

## NTai
aux_source_directory(NTai/AI/NTai/Units ntai)
aux_source_directory(NTai/AI/NTai/Units/Behaviours ntai)
aux_source_directory(NTai/AI/NTai/Tasks ntai)
aux_source_directory(NTai/AI/NTai/Helpers ntai)
aux_source_directory(NTai/AI/NTai/Helpers/grid ntai)
aux_source_directory(NTai/AI/NTai/Helpers/Terrain ntai)
aux_source_directory(NTai/AI/NTai/Helpers/Units ntai)
aux_source_directory(NTai/AI/NTai/Engine ntai)
aux_source_directory(NTai/AI/NTai/Core ntai)
aux_source_directory(NTai/AI/NTai/Agents ntai)
#add_library(NTai SHARED ${ntai} ${legacycppaienv})
#target_link_libraries(NTai boost_thread-mt)

## Null Legacy C++ AI
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI nullLegacyCppAI)
add_library(NullLegacyCppAI-0.1 MODULE ${nullLegacyCppAI} ${legacycppaienv} ${creg})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullLegacyCppAI-0.1)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullLegacyCppAI-0.1)

## Null C AI
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI nullAI)
add_library(NullAI-0.1 MODULE ${nullAI} ${aienv})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullAI-0.1)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullAI-0.1)

if (UNIX)
	# doesn't build in win32currently
	aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/JCAI jcai)
#	add_library(JCAI MODULE ${jcai} ${legacycppaienv})
endif (UNIX)

if (UNIX)
#	install (TARGETS KAIK-0.13 KAI-0.2 RAI-0.553 AAI NTai NullLegacyCppAI-0.1 NullAI-0.1 JCAI DESTINATION ${DATADIR}/AI/Skirmish/impls)
else (UNIX)
#	install (TARGETS KAIK-0.13 KAI-0.2 RAI-0.553 AAI NTai NullLegacyCppAI-0.1 NullAI-0.1 DESTINATION ${DATADIR}/AI/Skirmish/impls)
endif (UNIX)
install (TARGETS KAIK-0.13 RAI-0.553 NullLegacyCppAI-0.1 NullAI-0.1 DESTINATION ${DATADIR}/AI/Skirmish/impls)

