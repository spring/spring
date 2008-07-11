include_directories(${CMAKE_SOURCE_DIR}/rts ${CMAKE_SOURCE_DIR}/rts/ExternalAI ${CMAKE_SOURCE_DIR}/rts/System)

SET(BASE_FLAGS "-march=i686 -fsingle-precision-constant -frounding-math -fsignaling-nans -mieee-fp -mfpmath=387 -pipe -fno-strict-aliasing -fPIC -D_REENTRANT")
IF (UNIX)
	SET(BASE_FLAGS "${BASE_FLAGS} -pthread")
ENDIF (UNIX)
SET(CMAKE_CXX_FLAGS_DEBUG   "${BASE_FLAGS} -ggdb1 -O1 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG2  "${BASE_FLAGS} -ggdb2 -O0 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG3  "${BASE_FLAGS} -ggdb3 -O0 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "${BASE_FLAGS} ${VISIBILITY_HIDDEN} -O2 -DNDEBUG -fomit-frame-pointer")
SET(CMAKE_CXX_FLAGS_PROFILE   "${BASE_FLAGS} -O2 -pg -Wall -DNDEBUG")


### Global AIs
aux_source_directory(Global/KAI-0.2 kai02)
add_library(KAI-0.2 SHARED ${kai02})

aux_source_directory(Global/KAIK-0.13 kai013)
add_library(KAIK-0.13 SHARED ${kai013})

aux_source_directory(Global/RAI-0.553 rai)
add_library(RAI_0.553 SHARED ${rai})

aux_source_directory(Global/AAI aai)
add_library(AAI SHARED ${aai})

aux_source_directory(Global/JCAI jcai)
add_library(JCAI SHARED ${jcai})

aux_source_directory(Global/NTai/AI/NTai/Units ntai)
aux_source_directory(Global/NTai/AI/NTai/Units/Behaviours ntai)
aux_source_directory(Global/NTai/AI/NTai/Tasks ntai)
aux_source_directory(Global/NTai/AI/NTai/Helpers ntai)
aux_source_directory(Global/NTai/AI/NTai/Helpers/Grid ntai)
aux_source_directory(Global/NTai/AI/NTai/Helpers/Terrain ntai)
aux_source_directory(Global/NTai/AI/NTai/Helpers/Units ntai)
aux_source_directory(Global/NTai/AI/NTai/Engine ntai)
aux_source_directory(Global/NTai/AI/NTai/Core ntai)
aux_source_directory(Global/NTai/AI/NTai/Agents ntai)
add_library(NTai SHARED ${ntai})

aux_source_directory(Global/TestGlobalAI testai)
add_library(TestGlobalAI SHARED ${testai})


install (TARGETS KAIK-0.13 KAI-0.2 RAI_0.553 AAI JCAI NTai TestGlobalAI DESTINATION spring/AI/Bot-libs)


### Group AIs
aux_source_directory(Group/CentralBuildAI central)
add_library(CentralBuildAI SHARED ${central})

aux_source_directory(Group/EconomyAI economy)
add_library(EconomyAI SHARED ${economy})

aux_source_directory(Group/MetalMakerAI metal)
add_library(MetalMakerAI SHARED ${metal})

aux_source_directory(Group/MexUpgraderAI mex)
add_library(MexUpgraderAI SHARED ${mex})

aux_source_directory(Group/RadarAI radar)
add_library(RadarAI SHARED ${radar})

aux_source_directory(Group/ReportIdleAI idle)
add_library(ReportIdleAI SHARED ${idle})

aux_source_directory(Group/SimpleFormationAI formation)
add_library(SimpleFormationAI SHARED ${formation})


install (TARGETS CentralBuildAI EconomyAI MetalMakerAI MexUpgraderAI RadarAI ReportIdleAI SimpleFormationAI DESTINATION spring/AI/Helper-libs)
