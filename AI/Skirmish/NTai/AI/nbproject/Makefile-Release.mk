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
	build/Release/GNU-Windows/NTai/Core/GlobalAI.o \
	build/Release/GNU-Windows/NTai/Agents/Scouter.o \
	build/Release/GNU-Windows/NTai/Helpers/Terrain/DTHandler.o \
	build/Release/GNU-Windows/NTai/Helpers/TdfParser.o \
	build/Release/GNU-Windows/NTai/Helpers/Units/Actions.o \
	build/Release/GNU-Windows/NTai/Helpers/grid/CGridManager.o \
	build/Release/GNU-Windows/NTai/Core/helper.o \
	build/Release/GNU-Windows/NTai/Helpers/Terrain/CBuildingPlacer.o \
	build/Release/GNU-Windows/NTai/Tasks/CLeaveBuildSpotTask.o \
	build/Release/GNU-Windows/NTai/Core/Interface.o \
	build/Release/GNU-Windows/NTai/Helpers/Terrain/MetalMap.o \
	build/Release/GNU-Windows/NTai/Helpers/Log.o \
	build/Release/GNU-Windows/NTai/Helpers/CEconomy.o \
	build/Release/GNU-Windows/NTai/Tasks/CConsoleTask.o \
	build/Release/GNU-Windows/NTai/Tasks/CKeywordConstructionTask.o \
	build/Release/GNU-Windows/NTai/Core/IModule.o \
	build/Release/GNU-Windows/NTai/Agents/Assigner.o \
	build/Release/GNU-Windows/NTai/Agents/CManufacturer.o \
	build/Release/GNU-Windows/NTai/Engine/COrderRouter.o \
	build/Release/GNU-Windows/NTai/Helpers/Terrain/MetalHandler.o \
	build/Release/GNU-Windows/NTai/Core/CMessage.o \
	build/Release/GNU-Windows/NTai/Agents/Chaser.o \
	build/Release/GNU-Windows/NTai/Agents/Planning.o \
	build/Release/GNU-Windows/NTai/Helpers/Units/CUnitDefLoader.o \
	build/Release/GNU-Windows/NTai/Helpers/Terrain/RadarHandler.o \
	build/Release/GNU-Windows/NTai/Tasks/CUnitConstructionTask.o \
	build/Release/GNU-Windows/NTai/Helpers/grid/CGridCell.o \
	build/Release/GNU-Windows/NTai/Helpers/Terrain/Map.o \
	build/Release/GNU-Windows/NTai/Helpers/ubuild.o \
	build/Release/GNU-Windows/NTai/Units/CUnit.o \
	build/Release/GNU-Windows/NTai/Helpers/Units/CUnitDefHelp.o \
	build/Release/GNU-Windows/NTai/Helpers/mtrand.o

# buggy mingw 4.2.1 tries to include rts/map as <map>, results in
# several screens of compilation errors
# provide paths to your system includes as a workaround
#SPRING_INCLUDES=-Ic:/mingw/include -Ic:/mingw/include/c++/4.2.1 -I../../../../rts -I../../../../rts/ExternalAI -I../../../../rts/System -I../../../../mingwlibs/include
SPRING_INCLUDES=-I../../../../rts -I../../../../rts/ExternalAI -I../../../../rts/System -I../../../../mingwlibs/include

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=\
	-L../../../../mingwlibs/lib \
	../../../../mingwlibs/lib/libboost_thread-mt.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS} dist/Release/GNU-Windows/ntai.dll

dist/Release/GNU-Windows/ntai.dll: ${OBJECTFILES}
	${MKDIR} -p dist/Release/GNU-Windows
	${LINK.cc} -shared -o dist/Release/GNU-Windows/ntai.dll -s ${OBJECTFILES} ${LDLIBSOPTIONS} 

build/Release/GNU-Windows/NTai/Core/GlobalAI.o: NTai/Core/GlobalAI.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Core
	$(COMPILE.cc) $(SPRING_INCLUDES) -O2 -s  -o build/Release/GNU-Windows/NTai/Core/GlobalAI.o NTai/Core/GlobalAI.cpp

build/Release/GNU-Windows/NTai/Agents/Scouter.o: NTai/Agents/Scouter.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Agents/Scouter.o NTai/Agents/Scouter.cpp

build/Release/GNU-Windows/NTai/Helpers/Terrain/DTHandler.o: NTai/Helpers/Terrain/DTHandler.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Terrain/DTHandler.o NTai/Helpers/Terrain/DTHandler.cpp

build/Release/GNU-Windows/NTai/Helpers/TdfParser.o: NTai/Helpers/TdfParser.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/TdfParser.o NTai/Helpers/TdfParser.cpp

build/Release/GNU-Windows/NTai/Helpers/Units/Actions.o: NTai/Helpers/Units/Actions.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Units
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Units/Actions.o NTai/Helpers/Units/Actions.cpp

build/Release/GNU-Windows/NTai/Helpers/grid/CGridManager.o: NTai/Helpers/grid/CGridManager.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/grid
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/grid/CGridManager.o NTai/Helpers/grid/CGridManager.cpp

build/Release/GNU-Windows/NTai/Core/helper.o: NTai/Core/helper.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Core
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Core/helper.o NTai/Core/helper.cpp

build/Release/GNU-Windows/NTai/Helpers/Terrain/CBuildingPlacer.o: NTai/Helpers/Terrain/CBuildingPlacer.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Terrain/CBuildingPlacer.o NTai/Helpers/Terrain/CBuildingPlacer.cpp

build/Release/GNU-Windows/NTai/Tasks/CLeaveBuildSpotTask.o: NTai/Tasks/CLeaveBuildSpotTask.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Tasks/CLeaveBuildSpotTask.o NTai/Tasks/CLeaveBuildSpotTask.cpp

build/Release/GNU-Windows/NTai/Core/Interface.o: NTai/Core/Interface.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Core
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Core/Interface.o NTai/Core/Interface.cpp

build/Release/GNU-Windows/NTai/Helpers/Terrain/MetalMap.o: NTai/Helpers/Terrain/MetalMap.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Terrain/MetalMap.o NTai/Helpers/Terrain/MetalMap.cpp

build/Release/GNU-Windows/NTai/Helpers/Log.o: NTai/Helpers/Log.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Log.o NTai/Helpers/Log.cpp

build/Release/GNU-Windows/NTai/Helpers/CEconomy.o: NTai/Helpers/CEconomy.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/CEconomy.o NTai/Helpers/CEconomy.cpp

build/Release/GNU-Windows/NTai/Tasks/CConsoleTask.o: NTai/Tasks/CConsoleTask.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Tasks/CConsoleTask.o NTai/Tasks/CConsoleTask.cpp

build/Release/GNU-Windows/NTai/Tasks/CKeywordConstructionTask.o: NTai/Tasks/CKeywordConstructionTask.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Tasks/CKeywordConstructionTask.o NTai/Tasks/CKeywordConstructionTask.cpp

build/Release/GNU-Windows/NTai/Core/IModule.o: NTai/Core/IModule.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Core
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Core/IModule.o NTai/Core/IModule.cpp

build/Release/GNU-Windows/NTai/Agents/Assigner.o: NTai/Agents/Assigner.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Agents/Assigner.o NTai/Agents/Assigner.cpp

build/Release/GNU-Windows/NTai/Agents/CManufacturer.o: NTai/Agents/CManufacturer.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Agents/CManufacturer.o NTai/Agents/CManufacturer.cpp

build/Release/GNU-Windows/NTai/Engine/COrderRouter.o: NTai/Engine/COrderRouter.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Engine
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Engine/COrderRouter.o NTai/Engine/COrderRouter.cpp

build/Release/GNU-Windows/NTai/Helpers/Terrain/MetalHandler.o: NTai/Helpers/Terrain/MetalHandler.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Terrain/MetalHandler.o NTai/Helpers/Terrain/MetalHandler.cpp

build/Release/GNU-Windows/NTai/Core/CMessage.o: NTai/Core/CMessage.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Core
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Core/CMessage.o NTai/Core/CMessage.cpp

build/Release/GNU-Windows/NTai/Agents/Chaser.o: NTai/Agents/Chaser.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Agents/Chaser.o NTai/Agents/Chaser.cpp

build/Release/GNU-Windows/NTai/Agents/Planning.o: NTai/Agents/Planning.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Agents
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Agents/Planning.o NTai/Agents/Planning.cpp

build/Release/GNU-Windows/NTai/Helpers/Units/CUnitDefLoader.o: NTai/Helpers/Units/CUnitDefLoader.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Units
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Units/CUnitDefLoader.o NTai/Helpers/Units/CUnitDefLoader.cpp

build/Release/GNU-Windows/NTai/Helpers/Terrain/RadarHandler.o: NTai/Helpers/Terrain/RadarHandler.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Terrain/RadarHandler.o NTai/Helpers/Terrain/RadarHandler.cpp

build/Release/GNU-Windows/NTai/Tasks/CUnitConstructionTask.o: NTai/Tasks/CUnitConstructionTask.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Tasks
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Tasks/CUnitConstructionTask.o NTai/Tasks/CUnitConstructionTask.cpp

build/Release/GNU-Windows/NTai/Helpers/grid/CGridCell.o: NTai/Helpers/grid/CGridCell.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/grid
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/grid/CGridCell.o NTai/Helpers/grid/CGridCell.cpp

build/Release/GNU-Windows/NTai/Helpers/Terrain/Map.o: NTai/Helpers/Terrain/Map.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Terrain
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Terrain/Map.o NTai/Helpers/Terrain/Map.cpp

build/Release/GNU-Windows/NTai/Helpers/ubuild.o: NTai/Helpers/ubuild.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/ubuild.o NTai/Helpers/ubuild.cpp

build/Release/GNU-Windows/NTai/Units/CUnit.o: NTai/Units/CUnit.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Units
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Units/CUnit.o NTai/Units/CUnit.cpp

build/Release/GNU-Windows/NTai/Helpers/Units/CUnitDefHelp.o: NTai/Helpers/Units/CUnitDefHelp.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers/Units
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/Units/CUnitDefHelp.o NTai/Helpers/Units/CUnitDefHelp.cpp

build/Release/GNU-Windows/NTai/Helpers/mtrand.o: NTai/Helpers/mtrand.cpp 
	${MKDIR} -p build/Release/GNU-Windows/NTai/Helpers
	$(COMPILE.cc) -O2 -s $(SPRING_INCLUDES) -o build/Release/GNU-Windows/NTai/Helpers/mtrand.o NTai/Helpers/mtrand.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/GNU-Windows/ntai.dll

# Subprojects
.clean-subprojects:
