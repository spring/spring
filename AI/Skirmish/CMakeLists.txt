
### Skirmish AIs
#remove_definitions(-DBUILDING_AI_INTERFACE -DSTREFLOP_X87 -DSTREFLOP_SSE -DSTREFLOP_SOFT)
add_definitions(-DBUILDING_AI)

aux_source_directory(KAI-0.2 kai02)
#add_library(KAI-0.2 SHARED ${kai02} ${legacycppaienv} ${creg})

aux_source_directory(KAIK-0.13 kaik013)
add_library(KAIK-0.13 SHARED ${kaik013} ${legacycppaienv} ${creg})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/KAIK-0.13)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/KAIK-0.13)

aux_source_directory(RAI-0.553 rai)
add_library(RAI-0.553 SHARED ${rai} ${legacycppaienv} RAI-0.553/KrogsMetalClass/MetalMap)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/RAI-0.553)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/RAI-0.553)

aux_source_directory(AAI aai)
#add_library(AAI SHARED ${aai} ${legacycppaienv})

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

#aux_source_directory(TestGlobalAI testai)
#add_library(TestGlobalAI SHARED ${testai} ${legacycppaienv})

aux_source_directory(NullAI nullai)
add_library(NullAI-0.1 SHARED ${nullai}  ${legacycppaienv})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullAI-0.1)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullAI-0.1)

if (UNIX)
	# doesn't build in win32currently
	aux_source_directory(JCAI jcai)
#	add_library(JCAI SHARED ${jcai} ${legacycppaienv})
endif (UNIX)

if (UNIX)
#	install (TARGETS KAIK-0.13 KAI-0.2 RAI-0.553 AAI NTai TestGlobalAI NullAI-0.1 JCAI DESTINATION ${DATADIR}/AI/Skirmish/impls)
else (UNIX)
#	install (TARGETS KAIK-0.13 KAI-0.2 RAI-0.553 AAI NTai TestGlobalAI NullAI-0.1 DESTINATION ${DATADIR}/AI/Skirmish/impls)
endif (UNIX)
install (TARGETS KAIK-0.13 RAI-0.553 NullAI-0.1 DESTINATION ${DATADIR}/AI/Skirmish/impls)

