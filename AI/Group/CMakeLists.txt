
### Group AIs
aux_source_directory(CentralBuildAI central)
#add_library(CentralBuildAI MODULE ${central})

aux_source_directory(EconomyAI economy)
#add_library(EconomyAI MODULE ${economy} ${aienv})

aux_source_directory(MetalMakerAI metal)
#add_library(MetalMakerAI MODULE ${metal})

aux_source_directory(MexUpgraderAI mex)
#add_library(MexUpgraderAI MODULE ${mex} ${aienv})

aux_source_directory(RadarAI radar)
#add_library(RadarAI MODULE ${radar})

aux_source_directory(ReportIdleAI idle)
#add_library(ReportIdleAI MODULE ${idle})

aux_source_directory(SimpleFormationAI formation)
#add_library(SimpleFormationAI MODULE ${formation})


#install (TARGETS CentralBuildAI EconomyAI MetalMakerAI MexUpgraderAI RadarAI ReportIdleAI SimpleFormationAI DESTINATION ${DATADIR}/AI/Helper-libs)
