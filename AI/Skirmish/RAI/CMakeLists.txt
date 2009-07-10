### RAI Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(RAI_SRC "${SKIRMISH_SRC}/RAI")
set(RAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if (BUILD_C_AIINTERFACE)
	set(BUILD_RAI TRUE)
else (BUILD_C_AIINTERFACE)
	set(BUILD_RAI FALSE)
	message ("warning: RAI Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if (BUILD_RAI)
	set(RAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${RAI_SRC}/VERSION)
		file(STRINGS "${RAI_SRC}/VERSION" RAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${RAI_SRC}/VERSION)
	set(RAI_TARGET "RAI")
	set(RAI_DATA_DIR ${SKIRMISH_DATA}/RAI/${RAI_VERS})
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: RAI ${RAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${RAI_SRC}/data/"
			DESTINATION "${RAI_DATA_DIR}")

	aux_source_directory(${RAI_SRC} rai)
	add_library(${RAI_TARGET} MODULE ${rai} ${cutilsenv})
	target_link_libraries(${RAI_TARGET} legacycpp)
	set_target_properties(${RAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	#set_target_properties(${RAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "RAI")
	install (TARGETS ${RAI_TARGET} DESTINATION ${RAI_DATA_DIR})
endif (BUILD_RAI)

