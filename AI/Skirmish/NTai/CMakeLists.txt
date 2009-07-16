### NTai Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

# AI specific properties
set(myName                   "NTai")
set(myDir                    ${CMAKE_CURRENT_SOURCE_DIR})
set(mySourceDir             ${myDir}/AI/NTai)
set(myAdditionalSources      ${CUtils_SRC})
set(myAdditionalCompileFlags "")
set(myAdditionalLibraries    legacycpp ${Boost_THREAD_LIBRARY})

aux_source_directory(${mySourceDir}/Units            mySources)
aux_source_directory(${mySourceDir}/Units/Behaviours mySources)
aux_source_directory(${mySourceDir}/Tasks            mySources)
aux_source_directory(${mySourceDir}/Helpers          mySources)
aux_source_directory(${mySourceDir}/Helpers/grid     mySources)
aux_source_directory(${mySourceDir}/Helpers/Terrain  mySources)
aux_source_directory(${mySourceDir}/Helpers/Units    mySources)
aux_source_directory(${mySourceDir}/Engine           mySources)
aux_source_directory(${mySourceDir}/Core             mySources)
aux_source_directory(${mySourceDir}/Agents           mySources)


# Check if the user wants to compile the AI
if    (BUILD_C_AIINTERFACE)
	set(BUILD_THIS_SKIRMISHAI TRUE)
else  (BUILD_C_AIINTERFACE)
	set(BUILD_THIS_SKIRMISHAI FALSE)
	message("warning: ${myName} Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)


# Compile and install
if    (BUILD_THIS_SKIRMISHAI)
	# Assemble meta data
	GetVersionFromFile(myVersion "${myDir}/VERSION")
	set(myTarget  "${myName}")
	set(myDataDir "${SKIRMISH_DATA}/${myName}/${myVersion}")
	SkirmishAIMessage(STATUS "Found Skirmish AI: ${myName} ${myVersion}")

	# Compile the library
	add_library(${myTarget} MODULE ${mySources} ${myAdditionalSources})
	target_link_libraries(${myTarget} ${myAdditionalLibraries})
	set_target_properties(${myTarget} PROPERTIES COMPILE_FLAGS "${myAdditionalCompileFlags}")
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME   "SkirmishAI")

	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION "${myDataDir}")
	# Install the library
	install(TARGETS   ${myTarget}      DESTINATION "${myDataDir}")
endif (BUILD_THIS_SKIRMISHAI)
