### CppTestAI Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(CppTestAI_SRC "${SKIRMISH_SRC}/CppTestAI")
set(CppTestAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if    (BUILD_C_AIINTERFACE)
	set(BUILD_CppTestAI TRUE)
else  (BUILD_C_AIINTERFACE)
	set(BUILD_CppTestAI FALSE)
	message ("warning: CppTestAI Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if    (BUILD_CppTestAI)
	set(CppTestAI_VERS "UNKNOWN_VERSION")
	if    (EXISTS ${CppTestAI_SRC}/VERSION)
		file(STRINGS "${CppTestAI_SRC}/VERSION" CppTestAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${CppTestAI_SRC}/VERSION)
	#set(CppTestAI_TARGET "CppTestAI-${CppTestAI_VERS}")
	set(CppTestAI_TARGET "CppTestAI")
	set(CppTestAI_DATA_DIR ${SKIRMISH_DATA}/CppTestAI/${CppTestAI_VERS})
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: CppTestAI ${CppTestAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${CppTestAI_SRC}/data/"
			DESTINATION "${CppTestAI_DATA_DIR}")

	aux_source_directory(${CppTestAI_SRC} cpptestai)
	include_directories(BEFORE ${CPPWRAPPER_GENERATED_SRC_DIR})
	add_library(${CppTestAI_TARGET} MODULE ${cpptestai} ${cutilsenv})
	target_link_libraries(${CppTestAI_TARGET} cppaiwrapper)
	set_target_properties(${CppTestAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	install (TARGETS ${CppTestAI_TARGET} DESTINATION ${CppTestAI_DATA_DIR})
endif (BUILD_CppTestAI)

