### Skirmish AIs
add_definitions(-DBUILDING_AI)

set(SKIRMISH_SRC "${CMAKE_SOURCE_DIR}/AI/Skirmish")
set(SKIRMISH_LIBS "${DATADIR}/AI/Skirmish/impls")
set(SKIRMISH_DATA "${DATADIR}/AI/Skirmish/data")


## KAI
set(KAI_SRC "${SKIRMISH_SRC}/KAI")
if (EXISTS ${KAI_SRC})
	set(KAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${KAI_SRC}/VERSION)
		file(STRINGS "${KAI_SRC}/VERSION" KAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${KAI_SRC}/VERSION)
	set(KAI_TARGET "KAI-${KAI_VERS}")
	message("${KAI_VERS}\t(KAI Version)")

	aux_source_directory(${KAI_SRC} kai)
	#add_library(${KAI_TARGET} MODULE ${kai} ${legacycppaienv} ${creg})
	#install (FILES ${KAI_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/KAI/${KAI_VERS})
	#install (FILES ${KAI_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/KAI/${KAI_VERS})
	#set_target_properties(${KAI_TARGET} PROPERTIES COMPILE_FLAGS "-DUSING_CREG")
	#install (TARGETS ${KAI_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${KAI_SRC})


## KAIK
set(KAIK_SRC "${SKIRMISH_SRC}/KAIK")
if (EXISTS ${KAIK_SRC})
	set(KAIK_VERS "UNKNOWN_VERSION")
	if (EXISTS ${KAIK_SRC}/VERSION)
		file(STRINGS "${KAIK_SRC}/VERSION" KAIK_VERS LIMIT_COUNT 1)
	endif (EXISTS ${KAIK_SRC}/VERSION)
	set(KAIK_TARGET "KAIK-${KAIK_VERS}")
	message("${KAIK_VERS}\t(KAIK Version)")

	aux_source_directory(${KAIK_SRC} kaik)
	add_library(${KAIK_TARGET} MODULE ${kaik} ${legacycppaienv} ${creg})
	install (FILES ${KAIK_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/KAIK/${KAIK_VERS})
	install (FILES ${KAIK_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/KAIK/${KAIK_VERS})
	set_target_properties(${KAIK_TARGET} PROPERTIES COMPILE_FLAGS "-DUSING_CREG")
	install (TARGETS ${KAIK_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${KAIK_SRC})


## RAI
set(RAI_SRC "${SKIRMISH_SRC}/RAI")
if (EXISTS ${RAI_SRC})
	set(RAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${RAI_SRC}/VERSION)
		file(STRINGS "${RAI_SRC}/VERSION" RAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${RAI_SRC}/VERSION)
	set(RAI_TARGET "RAI-${RAI_VERS}")
	message("${RAI_VERS}\t(RAI Version)")

	aux_source_directory(${RAI_SRC} rai)
	add_library(${RAI_TARGET} MODULE ${rai} ${legacycppaienv})
	install (FILES ${RAI_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/RAI/${RAI_VERS})
	install (FILES ${RAI_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/RAI/${RAI_VERS})
	install (TARGETS ${RAI_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${RAI_SRC})


## AAI
set(AAI_SRC "${SKIRMISH_SRC}/AAI")
if (EXISTS ${AAI_SRC})
	set(AAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${AAI_SRC}/VERSION)
		file(STRINGS "${AAI_SRC}/VERSION" AAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${AAI_SRC}/VERSION)
	set(AAI_TARGET "AAI-${AAI_VERS}")
	message("${AAI_VERS}\t(AAI Version)")

	aux_source_directory(${AAI_SRC} aai)
	#add_library(${AAI_TARGET} MODULE ${aai} ${legacycppaienv})
	#install (FILES ${AAI_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/AAI/${AAI_VERS})
	#install (FILES ${AAI_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/AAI/${AAI_VERS})
	#install (TARGETS ${AAI_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${AAI_SRC})


## NTai
set(NTai_SRC "${SKIRMISH_SRC}/NTai")
if (EXISTS ${NTai_SRC})
	set(NTai_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NTai_SRC}/VERSION)
		file(STRINGS "${NTai_SRC}/VERSION" NTai_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NTai_SRC}/VERSION)
	set(NTai_TARGET "NTai-${NTai_VERS}")
	message("${NTai_VERS}\t(NTai Version)")

	aux_source_directory(${NTai_SRC} ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Units ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Units/Behaviours ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Tasks ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers/grid ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers/Terrain ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers/Units ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Engine ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Core ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Agents ntai)
	#add_library(${NTai_TARGET} MODULE ${ntai} ${legacycppaienv})
	#target_link_libraries(${NTai_TARGET} boost_thread-mt)
	#install (FILES ${NTai_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/NTai/${NTai_VERS})
	#install (FILES ${NTai_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/NTai/${NTai_VERS})
	#install (TARGETS ${NTai_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${NTai_SRC})


## Null Legacy C++ AI
set(NullLegacyCppAI_SRC "${SKIRMISH_SRC}/NullLegacyCppAI")
if (EXISTS ${NullLegacyCppAI_SRC})
	set(NullLegacyCppAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NullLegacyCppAI_SRC}/VERSION)
		file(STRINGS "${NullLegacyCppAI_SRC}/VERSION" NullLegacyCppAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullLegacyCppAI_SRC}/VERSION)
	set(NullLegacyCppAI_TARGET "NullLegacyCppAI-${NullLegacyCppAI_VERS}")
	message("${NullLegacyCppAI_VERS}\t(NullLegacyCppAI Version)")

	aux_source_directory(${NullLegacyCppAI_SRC} nulllegacycppai)
	add_library(${NullLegacyCppAI_TARGET} MODULE ${nulllegacycppai} ${legacycppaienv})
	install (FILES ${NullLegacyCppAI_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/NullLegacyCppAI/${NullLegacyCppAI_VERS})
	install (FILES ${NullLegacyCppAI_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/NullLegacyCppAI/${NullLegacyCppAI_VERS})
	install (TARGETS ${NullLegacyCppAI_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${NullLegacyCppAI_SRC})


## Null C AI
set(NullAI_SRC "${SKIRMISH_SRC}/NullAI")
if (EXISTS ${NullAI_SRC})
	set(NullAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NullAI_SRC}/VERSION)
		file(STRINGS "${NullAI_SRC}/VERSION" NullAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullAI_SRC}/VERSION)
	set(NullAI_TARGET "NullAI-${NullAI_VERS}")
	message("${NullAI_VERS}\t(NullAI Version)")

	aux_source_directory(${NullAI_SRC} nullai)
	add_library(${NullAI_TARGET} MODULE ${mySrc} ${nullai})
	install (FILES ${NullAI_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/NullAI/${NullAI_VERS})
	install (FILES ${NullAI_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/NullAI/${NullAI_VERS})
	install (TARGETS ${NullAI_TARGET} DESTINATION ${SKIRMISH_LIBS})
endif (EXISTS ${NullAI_SRC})


## JCAI
# does currently not build in win32
if (UNIX)
	set(JCAI_SRC "${SKIRMISH_SRC}/JCAI")
	if (EXISTS ${JCAI_SRC})
		set(JCAI_VERS "UNKNOWN_VERSION")
		if (EXISTS ${JCAI_SRC}/VERSION)
			file(STRINGS "${JCAI_SRC}/VERSION" JCAI_VERS LIMIT_COUNT 1)
		endif (EXISTS ${JCAI_SRC}/VERSION)
		set(JCAI_TARGET "JCAI-${JCAI_VERS}")
		message("${JCAI_VERS}\t(JCAI Version)")

		aux_source_directory(${JCAI_SRC} jcai)
		#add_library(${JCAI_TARGET} MODULE ${jcai} ${legacycppaienv})
		#install (FILES ${JCAI_SRC}/AIInfo.lua DESTINATION ${SKIRMISH_DATA}/JCAI/${JCAI_VERS})
		#install (FILES ${JCAI_SRC}/AIOptions.lua DESTINATION ${SKIRMISH_DATA}/JCAI/${JCAI_VERS})
		#install (TARGETS ${JCAI_TARGET} DESTINATION ${SKIRMISH_LIBS})
	endif (EXISTS ${JCAI_SRC})
endif (UNIX)

