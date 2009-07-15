### NullLegacyCppAI Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(NullLegacyCppAI_SRC "${SKIRMISH_SRC}/NullLegacyCppAI")
set(NullLegacyCppAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if    (BUILD_C_AIINTERFACE)
	set(BUILD_NullLegacyCppAI TRUE)
else  (BUILD_C_AIINTERFACE)
	set(BUILD_NullLegacyCppAI FALSE)
	message ("warning: NullLegacyCppAI Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if    (BUILD_NullLegacyCppAI)
	set(NullLegacyCppAI_VERS "UNKNOWN_VERSION")
	if    (EXISTS ${NullLegacyCppAI_SRC}/VERSION)
		file(STRINGS "${NullLegacyCppAI_SRC}/VERSION" NullLegacyCppAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullLegacyCppAI_SRC}/VERSION)
	#set(NullLegacyCppAI_TARGET "NullLegacyCppAI-${NullLegacyCppAI_VERS}")
	set(NullLegacyCppAI_TARGET "NullLegacyCppAI")
	set(NullLegacyCppAI_DATA_DIR ${SKIRMISH_DATA}/NullLegacyCppAI/${NullLegacyCppAI_VERS})
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullLegacyCppAI ${NullLegacyCppAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullLegacyCppAI_SRC}/data/"
			DESTINATION "${NullLegacyCppAI_DATA_DIR}")

	aux_source_directory(${NullLegacyCppAI_SRC} nulllegacycppai)
	add_library(${NullLegacyCppAI_TARGET} MODULE ${nulllegacycppai} ${cutilsenv})
	target_link_libraries(${NullLegacyCppAI_TARGET} legacycpp)
	set_target_properties(${NullLegacyCppAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	#set_target_properties(${NullLegacyCppAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "NullLegacyCppAI")
	install (TARGETS ${NullLegacyCppAI_TARGET} DESTINATION ${NullLegacyCppAI_DATA_DIR})
endif (BUILD_NullLegacyCppAI)

