include_directories(${CMAKE_SOURCE_DIR}/rts ${CMAKE_SOURCE_DIR}/rts/ExternalAI ${CMAKE_SOURCE_DIR}/rts/System)

# This way, we have the build-dir structure more like the install-dir one,
# which allows for testing Global AIs in the build dir, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "Bot-libs")

remove_definitions(-DSTREFLOP_SSE) # would break AI compiling
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1) # -DUSING_CREG)
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
	install (TARGETS RAI DESTINATION ${AIDIR}/AI/Bot-libs)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/RAI)

aux_source_directory(Global/AAI aai)
add_library(AAI MODULE ${aai})

if (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/JCAI)
	if (UNIX)
		# doesn't build in win32currently
		aux_source_directory(Global/JCAI jcai)
		add_library(JCAI MODULE ${jcai})
		install (TARGETS JCAI DESTINATION ${AIDIR}/AI/Bot-libs)
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
	install (TARGETS NTai DESTINATION ${AIDIR}/AI/Bot-libs)
endif (EXISTS ${CMAKE_SOURCE_DIR}/AI/Global/NTai)

aux_source_directory(Global/TestGlobalAI testai)
add_library(TestGlobalAI MODULE ${testai})

install (TARGETS KAIK-0.13 KAI-0.2 AAI TestGlobalAI DESTINATION ${AIDIR}/AI/Bot-libs)

### Group AIs
aux_source_directory(Group/CentralBuildAI central)
add_library(CentralBuildAI MODULE ${central})
set_target_properties(CentralBuildAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")

aux_source_directory(Group/EconomyAI economy)
add_library(EconomyAI MODULE ${economy} ${creg})
set_target_properties(EconomyAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")

aux_source_directory(Group/MetalMakerAI metal)
add_library(MetalMakerAI MODULE ${metal})
set_target_properties(MetalMakerAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")

aux_source_directory(Group/MexUpgraderAI mex)
add_library(MexUpgraderAI MODULE ${mex} ${creg})
set_target_properties(MexUpgraderAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")

aux_source_directory(Group/RadarAI radar)
add_library(RadarAI MODULE ${radar})
set_target_properties(RadarAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")

aux_source_directory(Group/ReportIdleAI idle)
add_library(ReportIdleAI MODULE ${idle})
set_target_properties(ReportIdleAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")

aux_source_directory(Group/SimpleFormationAI formation)
add_library(SimpleFormationAI MODULE ${formation})
set_target_properties(CentralBuildAI PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Helper-libs")


install (TARGETS CentralBuildAI EconomyAI MetalMakerAI MexUpgraderAI RadarAI ReportIdleAI SimpleFormationAI DESTINATION ${AIDIR}/AI/Helper-libs)
