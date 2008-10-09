### Skirmish AIs
add_definitions(-DBUILDING_AI)


## KAI
set(KAI_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAI/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/KAI/VERSION" KAI_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAI/VERSION)
message("${KAI_VERS}\t(KAI Version)")

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/KAI-0.2 kai02)
#add_library(KAI-${KAI_VERS} MODULE ${kai02} ${legacycppaienv} ${creg})
#set_target_properties(KAI-${KAI_VERS} PROPERTIES COMPILE_FLAGS "-DUSING_CREG")


## KAIK
set(KAIK_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/VERSION" KAIK_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/VERSION)
message("${KAIK_VERS}\t(KAIK Version)")

add_library("KAIK-${KAIK_VERS}" MODULE ${kaik013} ${legacycppaienv} ${creg})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/KAIK-${KAIK_VERS})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/KAIK-0.13/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/KAIK-${KAIK_VERS})
set_target_properties("KAIK-${KAIK_VERS}" PROPERTIES COMPILE_FLAGS "-DUSING_CREG")


## RAI
set(RAI_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/VERSION" RAI_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/VERSION)
message("${RAI_VERS}\t(RAI Version)")

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553 rai)
add_library(RAI-${RAI_VERS} MODULE ${rai} ${legacycppaienv} RAI-0.553/KrogsMetalClass/MetalMap)
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/RAI-${RAI_VERS})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/RAI-0.553/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/RAI-${RAI_VERS})


## AAI
set(AAI_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI/VERSION" AAI_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI/VERSION)
message("${AAI_VERS}\t(AAI Version)")

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI aai)
#add_library(AAI-${AAI_VERS} MODULE ${aai} ${legacycppaienv})
#install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/AAI-${AAI_VERS})
#install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/AAI/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/AAI-${AAI_VERS})


## NTai
set(NTai_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/NTai/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/NTai/VERSION" NTai_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/NTai/VERSION)
message("${NTai_VERS}\t(NTai Version)")

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
#target_link_libraries(NTai-${NTai_VERS} boost_thread-mt)
#install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NTai/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NTai-${NTai_VERS})
#install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NTai/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NTai-${NTai_VERS})


## Null Legacy C++ AI
set(NullLegacyCppAI_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/VERSION" NullLegacyCppAI_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/VERSION)
message("${NullLegacyCppAI_VERS}\t(NullLegacyCppAI Version)")

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI nullLegacyCppAI)
add_library(NullLegacyCppAI-${NullLegacyCppAI_VERS} MODULE ${nullLegacyCppAI} ${legacycppaienv})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullLegacyCppAI-${NullLegacyCppAI_VERS})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullLegacyCppAI/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullLegacyCppAI-${NullLegacyCppAI_VERS})


## Null C AI
set(NullAI_VERS "UNKNOWN_VERSION")
if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/VERSION" NullAI_VERS LIMIT_COUNT 1)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/VERSION)
message("${NullAI_VERS}\t(NullAI Version)")

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI nullAI)
add_library(NullAI-${NullAI_VERS} MODULE ${nullAI} ${aienv})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/AIInfo.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullAI-${NullAI_VERS})
install (FILES ${CMAKE_SOURCE_DIR}/AI/Skirmish/NullAI/AIOptions.lua DESTINATION ${DATADIR}/AI/Skirmish/data/NullAI-${NullAI_VERS})

## JCAI
if (UNIX)
	# doesn't build in win32currently
	set(JCAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/JCAI/VERSION)
		file(STRINGS "${CMAKE_SOURCE_DIR}/AI/Skirmish/JCAI/VERSION" JCAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Skirmish/JCAI/VERSION)
	message("${JCAI_VERS}\t(JCAI version)")
	
	aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Skirmish/JCAI jcai)
#	add_library(JCAI-${JCAI_VERS} MODULE ${jcai} ${legacycppaienv})
endif (UNIX)


## Instal all AIs
install (TARGETS KAIK-${KAIK_VERS} RAI-${RAI_VERS} NullLegacyCppAI-${NullLegacyCppAI_VERS} NullAI-${NullAI_VERS} DESTINATION ${DATADIR}/AI/Skirmish/impls)
#install (TARGETS KAI-${KAI_VERS} AAI-${AAI_VERS} NTai-${NTai_VERS} DESTINATION ${DATADIR}/AI/Skirmish/impls)
#if (UNIX)
#	install (TARGETS JCAI-${JCAI_VERS} DESTINATION ${DATADIR}/AI/Skirmish/impls)
#endif (UNIX)
