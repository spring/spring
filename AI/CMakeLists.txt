include_directories(${CMAKE_SOURCE_DIR}/rts ${CMAKE_SOURCE_DIR}/rts/ExternalAI ${CMAKE_SOURCE_DIR}/rts/System)

remove_definitions(-DSTREFLOP_SSE) # would break AI compiling
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1 -DUSING_CREG)
aux_source_directory(${CMAKE_SOURCE_DIR}/rts/System/creg creg)
list (APPEND creg ${CMAKE_SOURCE_DIR}/rts/System/float3)

### Global AIs
aux_source_directory(Global/KAI-0.2 kai02)
add_library(KAI-0.2 MODULE ${kai02} ${creg})

aux_source_directory(Global/KAIK-0.13 kai013)
add_library(KAIK-0.13 MODULE ${kai013} ${creg})

if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/RAI)
	aux_source_directory(Global/RAI rai)
	#aux_source_directory(Global/RAI/KrogsMetalClass rai)
	add_library(RAI MODULE ${rai})
	install (TARGETS RAI DESTINATION ${DATADIR}/AI/Bot-libs)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/RAI)

aux_source_directory(Global/AAI aai)
add_library(AAI MODULE ${aai})

if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/JCAI)
	if (UNIX)
		# doesn't build in win32currently
		aux_source_directory(Global/JCAI jcai)
		add_library(JCAI MODULE ${jcai})
		install (TARGETS JCAI DESTINATION ${DATADIR}/AI/Bot-libs)
	endif (UNIX)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/JCAI)

if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/NTai)
	aux_source_directory(Global/NTai/AI/NTai/Units ntai)
	aux_source_directory(Global/NTai/AI/NTai/Units/Behaviours ntai)
	aux_source_directory(Global/NTai/AI/NTai/Tasks ntai)
	aux_source_directory(Global/NTai/AI/NTai/Helpers ntai)
	aux_source_directory(Global/NTai/AI/NTai/Helpers/grid ntai)
	aux_source_directory(Global/NTai/AI/NTai/Helpers/Terrain ntai)
	aux_source_directory(Global/NTai/AI/NTai/Helpers/Units ntai)
	aux_source_directory(Global/NTai/AI/NTai/Engine ntai)
	aux_source_directory(Global/NTai/AI/NTai/Core ntai)
	aux_source_directory(Global/NTai/AI/NTai/Agents ntai)
	add_library(NTai MODULE ${ntai})
	target_link_libraries(NTai ${Boost_THREAD_LIBRARY})
	install (TARGETS NTai DESTINATION ${DATADIR}/AI/Bot-libs)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/NTai)

aux_source_directory(Global/TestGlobalAI testai)
add_library(TestGlobalAI MODULE ${testai})

install (TARGETS KAIK-0.13 KAI-0.2 AAI TestGlobalAI DESTINATION ${DATADIR}/AI/Bot-libs)

### Group AIs
aux_source_directory(Group/CentralBuildAI central)
add_library(CentralBuildAI MODULE ${central})

aux_source_directory(Group/EconomyAI economy)
add_library(EconomyAI MODULE ${economy} ${creg})

aux_source_directory(Group/MetalMakerAI metal)
add_library(MetalMakerAI MODULE ${metal})

aux_source_directory(Group/MexUpgraderAI mex)
add_library(MexUpgraderAI MODULE ${mex} ${creg})

aux_source_directory(Group/RadarAI radar)
add_library(RadarAI MODULE ${radar})

aux_source_directory(Group/ReportIdleAI idle)
add_library(ReportIdleAI MODULE ${idle})

aux_source_directory(Group/SimpleFormationAI formation)
add_library(SimpleFormationAI MODULE ${formation})


install (TARGETS CentralBuildAI EconomyAI MetalMakerAI MexUpgraderAI RadarAI ReportIdleAI SimpleFormationAI DESTINATION ${DATADIR}/AI/Helper-libs)
