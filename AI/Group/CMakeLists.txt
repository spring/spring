
### Group AIs
aux_source_directory(CentralBuildAI central)
add_library(CentralBuildAI SHARED ${central})

aux_source_directory(EconomyAI economy)
add_library(EconomyAI SHARED ${economy} ${aienv})

aux_source_directory(MetalMakerAI metal)
add_library(MetalMakerAI SHARED ${metal})

aux_source_directory(MexUpgraderAI mex)
add_library(MexUpgraderAI SHARED ${mex} ${aienv})

aux_source_directory(RadarAI radar)
add_library(RadarAI SHARED ${radar})

aux_source_directory(ReportIdleAI idle)
add_library(ReportIdleAI SHARED ${idle})

aux_source_directory(SimpleFormationAI formation)
add_library(SimpleFormationAI SHARED ${formation})


install (TARGETS CentralBuildAI EconomyAI MetalMakerAI MexUpgraderAI RadarAI ReportIdleAI SimpleFormationAI DESTINATION ${DATADIR}/AI/Helper-libs)
