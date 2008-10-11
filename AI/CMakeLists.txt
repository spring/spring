include_directories(${CMAKE_SOURCE_DIR}/rts ${CMAKE_SOURCE_DIR}/rts/ExternalAI ${CMAKE_SOURCE_DIR}/rts/System)

add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)
aux_source_directory(${CMAKE_SOURCE_DIR}/rts/System/creg creg)
list (APPEND creg ${CMAKE_SOURCE_DIR}/rts/System/float3)

### Global AIs
aux_source_directory(Global/KAI-0.2 kai02)
add_library(KAI-0.2 SHARED ${kai02} ${creg})

aux_source_directory(Global/KAIK-0.13 kai013)
add_library(KAIK-0.13 SHARED ${kai013} ${creg})

if (EXISTS Global/RAI-0.553)
	aux_source_directory(Global/RAI-0.553 rai)
	aux_source_directory(Global/RAI-0.553/KrogsMetalClass rai)
	add_library(RAI_0.553 SHARED ${rai})
	install (TARGETS RAI_0.553 DESTINATION ${DATADIR}/AI/Bot-libs)
endif (EXISTS Global/RAI-0.553)

aux_source_directory(Global/AAI aai)
add_library(AAI SHARED ${aai})

if (EXISTS Global/JCAI)
	if (UNIX)
		# doesn't build in win32currently
		aux_source_directory(Global/JCAI jcai)
		add_library(JCAI SHARED ${jcai})
		install (TARGETS JCAI DESTINATION ${DATADIR}/AI/Bot-libs)
	endif (UNIX)
endif (EXISTS Global/JCAI)

if (EXISTS Global/NTai)
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
	add_library(NTai SHARED ${ntai})
	target_link_libraries(NTai ${Boost_THREAD_LIBRARY})
	install (TARGETS NTai DESTINATION ${DATADIR}/AI/Bot-libs)
endif (EXISTS Global/NTai)

aux_source_directory(Global/TestGlobalAI testai)
add_library(TestGlobalAI SHARED ${testai})


if (UNIX)
	install (TARGETS KAIK-0.13 KAI-0.2 AAI TestGlobalAI DESTINATION ${DATADIR}/AI/Bot-libs)
else (UNIX)
	install (TARGETS KAIK-0.13 KAI-0.2 AAI TestGlobalAI DESTINATION ${DATADIR}/AI/Bot-libs)
endif (UNIX)

### Group AIs
aux_source_directory(Group/CentralBuildAI central)
add_library(CentralBuildAI SHARED ${central})

aux_source_directory(Group/EconomyAI economy)
add_library(EconomyAI SHARED ${economy} ${creg})

aux_source_directory(Group/MetalMakerAI metal)
add_library(MetalMakerAI SHARED ${metal})

aux_source_directory(Group/MexUpgraderAI mex)
add_library(MexUpgraderAI SHARED ${mex} ${creg})

aux_source_directory(Group/RadarAI radar)
add_library(RadarAI SHARED ${radar})

aux_source_directory(Group/ReportIdleAI idle)
add_library(ReportIdleAI SHARED ${idle})

aux_source_directory(Group/SimpleFormationAI formation)
add_library(SimpleFormationAI SHARED ${formation})


install (TARGETS CentralBuildAI EconomyAI MetalMakerAI MexUpgraderAI RadarAI ReportIdleAI SimpleFormationAI DESTINATION ${DATADIR}/AI/Helper-libs)
