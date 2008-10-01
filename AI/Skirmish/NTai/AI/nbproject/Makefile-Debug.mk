#
# Gererated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add custumized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=g77

# Include project Makefile
include Makefile

# Object Files
OBJECTFILES= \
	build/Debug/GNU-Windows/NTai/Core/GlobalAI.o \
	build/Debug/GNU-Windows/NTai/Agents/Scouter.o \
	build/Debug/GNU-Windows/NTai/Helpers/Terrain/DTHandler.o \
	build/Debug/GNU-Windows/NTai/Helpers/TdfParser.o \
	build/Debug/GNU-Windows/NTai/Helpers/Units/Actions.o \
	build/Debug/GNU-Windows/NTai/Helpers/grid/CGridManager.o \
	build/Debug/GNU-Windows/NTai/Core/helper.o \
	build/Debug/GNU-Windows/NTai/Helpers/Terrain/CBuildingPlacer.o \
	build/Debug/GNU-Windows/NTai/Tasks/CLeaveBuildSpotTask.o \
	build/Debug/GNU-Windows/NTai/Core/Interface.o \
	build/Debug/GNU-Windows/NTai/Helpers/Terrain/MetalMap.o \
	build/Debug/GNU-Windows/NTai/Helpers/Log.o \
	build/Debug/GNU-Windows/NTai/Helpers/CEconomy.o \
	build/Debug/GNU-Windows/NTai/Tasks/CConsoleTask.o \
	build/Debug/GNU-Windows/NTai/Tasks/CKeywordConstructionTask.o \
	build/Debug/GNU-Windows/NTai/Core/IModule.o \
	build/Debug/GNU-Windows/NTai/Agents/Assigner.o \
	build/Debug/GNU-Windows/NTai/Agents/CManufacturer.o \
	build/Debug/GNU-Windows/NTai/Engine/COrderRouter.o \
	build/Debug/GNU-Windows/NTai/Helpers/Terrain/MetalHandler.o \
	build/Debug/GNU-Windows/NTai/Core/CMessage.o \
	build/Debug/GNU-Windows/NTai/Agents/Chaser.o \
	build/Debug/GNU-Windows/NTai/Agents/Planning.o \
	build/Debug/GNU-Windows/NTai/Helpers/Units/CUnitDefLoader.o \
	build/Debug/GNU-Windows/NTai/Helpers/Terrain/RadarHandler.o \
	build/Debug/GNU-Windows/NTai/Tasks/CUnitConstructionTask.o \
	build/Debug/GNU-Windows/NTai/Helpers/grid/CGridCell.o \
	build/Debug/GNU-Windows/NTai/Helpers/Terrain/Map.o \
	build/Debug/GNU-Windows/NTai/Helpers/ubuild.o \
	build/Debug/GNU-Windows/NTai/Units/CUnit.o \
	build/Debug/GNU-Windows/NTai/Helpers/Units/CUnitDefHelp.o \
	build/Debug/GNU-Windows/NTai/Helpers/mtrand.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=\
	-LC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/lib \
	C:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/lib/libboost_thread-mt.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS} dist/Debug/GNU-Windows/NTai.dll

dist/Debug/GNU-Windows/NTai.dll: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/GNU-Windows
	${LINK.cc} -shared -o dist/Debug/GNU-Windows/NTai.dll ${OBJECTFILES} ${LDLIBSOPTIONS} 

build/Debug/GNU-Windows/NTai/Core/GlobalAI.o: NTai/Core/GlobalAI.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Core
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Core/GlobalAI.o NTai/Core/GlobalAI.cpp

build/Debug/GNU-Windows/NTai/Agents/Scouter.o: NTai/Agents/Scouter.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Agents/Scouter.o NTai/Agents/Scouter.cpp

build/Debug/GNU-Windows/NTai/Helpers/Terrain/DTHandler.o: NTai/Helpers/Terrain/DTHandler.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Terrain/DTHandler.o NTai/Helpers/Terrain/DTHandler.cpp

build/Debug/GNU-Windows/NTai/Helpers/TdfParser.o: NTai/Helpers/TdfParser.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/TdfParser.o NTai/Helpers/TdfParser.cpp

build/Debug/GNU-Windows/NTai/Helpers/Units/Actions.o: NTai/Helpers/Units/Actions.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Units
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Units/Actions.o NTai/Helpers/Units/Actions.cpp

build/Debug/GNU-Windows/NTai/Helpers/grid/CGridManager.o: NTai/Helpers/grid/CGridManager.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/grid
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/grid/CGridManager.o NTai/Helpers/grid/CGridManager.cpp

build/Debug/GNU-Windows/NTai/Core/helper.o: NTai/Core/helper.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Core
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Core/helper.o NTai/Core/helper.cpp

build/Debug/GNU-Windows/NTai/Helpers/Terrain/CBuildingPlacer.o: NTai/Helpers/Terrain/CBuildingPlacer.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Terrain/CBuildingPlacer.o NTai/Helpers/Terrain/CBuildingPlacer.cpp

build/Debug/GNU-Windows/NTai/Tasks/CLeaveBuildSpotTask.o: NTai/Tasks/CLeaveBuildSpotTask.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Tasks/CLeaveBuildSpotTask.o NTai/Tasks/CLeaveBuildSpotTask.cpp

build/Debug/GNU-Windows/NTai/Core/Interface.o: NTai/Core/Interface.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Core
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Core/Interface.o NTai/Core/Interface.cpp

build/Debug/GNU-Windows/NTai/Helpers/Terrain/MetalMap.o: NTai/Helpers/Terrain/MetalMap.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Terrain/MetalMap.o NTai/Helpers/Terrain/MetalMap.cpp

build/Debug/GNU-Windows/NTai/Helpers/Log.o: NTai/Helpers/Log.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Log.o NTai/Helpers/Log.cpp

build/Debug/GNU-Windows/NTai/Helpers/CEconomy.o: NTai/Helpers/CEconomy.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/CEconomy.o NTai/Helpers/CEconomy.cpp

build/Debug/GNU-Windows/NTai/Tasks/CConsoleTask.o: NTai/Tasks/CConsoleTask.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Tasks/CConsoleTask.o NTai/Tasks/CConsoleTask.cpp

build/Debug/GNU-Windows/NTai/Tasks/CKeywordConstructionTask.o: NTai/Tasks/CKeywordConstructionTask.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Tasks/CKeywordConstructionTask.o NTai/Tasks/CKeywordConstructionTask.cpp

build/Debug/GNU-Windows/NTai/Core/IModule.o: NTai/Core/IModule.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Core
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Core/IModule.o NTai/Core/IModule.cpp

build/Debug/GNU-Windows/NTai/Agents/Assigner.o: NTai/Agents/Assigner.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Agents/Assigner.o NTai/Agents/Assigner.cpp

build/Debug/GNU-Windows/NTai/Agents/CManufacturer.o: NTai/Agents/CManufacturer.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Agents/CManufacturer.o NTai/Agents/CManufacturer.cpp

build/Debug/GNU-Windows/NTai/Engine/COrderRouter.o: NTai/Engine/COrderRouter.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Engine
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Engine/COrderRouter.o NTai/Engine/COrderRouter.cpp

build/Debug/GNU-Windows/NTai/Helpers/Terrain/MetalHandler.o: NTai/Helpers/Terrain/MetalHandler.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Terrain/MetalHandler.o NTai/Helpers/Terrain/MetalHandler.cpp

build/Debug/GNU-Windows/NTai/Core/CMessage.o: NTai/Core/CMessage.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Core
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Core/CMessage.o NTai/Core/CMessage.cpp

build/Debug/GNU-Windows/NTai/Agents/Chaser.o: NTai/Agents/Chaser.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Agents/Chaser.o NTai/Agents/Chaser.cpp

build/Debug/GNU-Windows/NTai/Agents/Planning.o: NTai/Agents/Planning.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Agents/Planning.o NTai/Agents/Planning.cpp

build/Debug/GNU-Windows/NTai/Helpers/Units/CUnitDefLoader.o: NTai/Helpers/Units/CUnitDefLoader.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Units
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Units/CUnitDefLoader.o NTai/Helpers/Units/CUnitDefLoader.cpp

build/Debug/GNU-Windows/NTai/Helpers/Terrain/RadarHandler.o: NTai/Helpers/Terrain/RadarHandler.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Terrain/RadarHandler.o NTai/Helpers/Terrain/RadarHandler.cpp

build/Debug/GNU-Windows/NTai/Tasks/CUnitConstructionTask.o: NTai/Tasks/CUnitConstructionTask.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Tasks/CUnitConstructionTask.o NTai/Tasks/CUnitConstructionTask.cpp

build/Debug/GNU-Windows/NTai/Helpers/grid/CGridCell.o: NTai/Helpers/grid/CGridCell.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/grid
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/grid/CGridCell.o NTai/Helpers/grid/CGridCell.cpp

build/Debug/GNU-Windows/NTai/Helpers/Terrain/Map.o: NTai/Helpers/Terrain/Map.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Terrain/Map.o NTai/Helpers/Terrain/Map.cpp

build/Debug/GNU-Windows/NTai/Helpers/ubuild.o: NTai/Helpers/ubuild.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/ubuild.o NTai/Helpers/ubuild.cpp

build/Debug/GNU-Windows/NTai/Units/CUnit.o: NTai/Units/CUnit.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Units
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Units/CUnit.o NTai/Units/CUnit.cpp

build/Debug/GNU-Windows/NTai/Helpers/Units/CUnitDefHelp.o: NTai/Helpers/Units/CUnitDefHelp.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers/Units
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/Units/CUnitDefHelp.o NTai/Helpers/Units/CUnitDefHelp.cpp

build/Debug/GNU-Windows/NTai/Helpers/mtrand.o: NTai/Helpers/mtrand.cpp 
	${MKDIR} -p build/Debug/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -g -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/ExternalAI -IC:/Users/Tom/Documents/Development/SPRING_SVN_TRUNK/rts/System -IC:/Users/Tom/Documents/Development/taspring/spring_folder/minglibs/mingwlibs/include -o build/Debug/GNU-Windows/NTai/Helpers/mtrand.o NTai/Helpers/mtrand.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/GNU-Windows/NTai.dll

# Subprojects
.clean-subprojects:
