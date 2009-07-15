### AAI Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(AAI_SRC "${SKIRMISH_SRC}/AAI")
set(AAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if (BUILD_C_AIINTERFACE)
	set(BUILD_AAI TRUE)
else (BUILD_C_AIINTERFACE)
	set(BUILD_AAI FALSE)
	message ("warning: AAI Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if (BUILD_AAI)
	set(AAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${AAI_SRC}/VERSION)
		file(STRINGS "${AAI_SRC}/VERSION" AAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${AAI_SRC}/VERSION)
	#set(AAI_TARGET "AAI-${AAI_VERS}")
	set(AAI_TARGET "AAI")
	set(AAI_DATA_DIR ${SKIRMISH_DATA}/AAI/${AAI_VERS})
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: AAI ${AAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${AAI_SRC}/data/"
			DESTINATION "${AAI_DATA_DIR}")

	aux_source_directory(${AAI_SRC} aai)
	add_library(${AAI_TARGET} MODULE ${aai} ${cutilsenv})
	target_link_libraries(${AAI_TARGET} legacycpp)
	set_target_properties(${AAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	#set_target_properties(${AAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "AAI")
	install (TARGETS ${AAI_TARGET} DESTINATION ${AAI_DATA_DIR})
endif (BUILD_AAI)

