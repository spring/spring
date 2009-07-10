### NullAI Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(NullAI_SRC "${SKIRMISH_SRC}/NullAI")
set(NullAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if    (BUILD_C_AIINTERFACE)
	set(BUILD_NullAI TRUE)
else  (BUILD_C_AIINTERFACE)
	set(BUILD_NullAI FALSE)
	message ("warning: NullAI Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if    (BUILD_NullAI)
	set(NullAI_VERS "UNKNOWN_VERSION")
	if    (EXISTS ${NullAI_SRC}/VERSION)
		file(STRINGS "${NullAI_SRC}/VERSION" NullAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullAI_SRC}/VERSION)
	#set(NullAI_TARGET "NullAI-${NullAI_VERS}")
	set(NullAI_TARGET "NullAI")
	set(NullAI_DATA_DIR ${SKIRMISH_DATA}/NullAI/${NullAI_VERS})
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullAI ${NullAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullAI_SRC}/data/"
			DESTINATION "${NullAI_DATA_DIR}")

	aux_source_directory(${NullAI_SRC} nullai)
	add_library(${NullAI_TARGET} MODULE ${mySrc} ${nullai})
	set_target_properties(${NullAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	#set_target_properties(${NullAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "NullAI")
	install (TARGETS ${NullAI_TARGET} DESTINATION ${NullAI_DATA_DIR})
endif (BUILD_NullAI)

