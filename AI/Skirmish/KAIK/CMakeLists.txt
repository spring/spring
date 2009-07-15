### KAIK Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(KAIK_SRC "${SKIRMISH_SRC}/KAIK")
set(KAIK_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if (BUILD_C_AIINTERFACE)
	set(BUILD_KAIK TRUE)
else (BUILD_C_AIINTERFACE)
	set(BUILD_KAIK FALSE)
	message ("warning: KAIK Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if (BUILD_KAIK)
	set(KAIK_VERS "UNKNOWN_VERSION")
	if (EXISTS ${KAIK_SRC}/VERSION)
		file(STRINGS "${KAIK_SRC}/VERSION" KAIK_VERS LIMIT_COUNT 1)
	endif (EXISTS ${KAIK_SRC}/VERSION)
	set(KAIK_TARGET "KAIK")
	set(KAIK_DATA_DIR ${SKIRMISH_DATA}/KAIK/${KAIK_VERS})
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: KAIK ${KAIK_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${KAIK_SRC}/data/"
			DESTINATION "${KAIK_DATA_DIR}")

	aux_source_directory(${KAIK_SRC} kaik)
	add_library(${KAIK_TARGET} MODULE ${kaik} ${cutilsenv})
	target_link_libraries(${KAIK_TARGET} legacycpp-creg)
	set_target_properties(${KAIK_TARGET} PROPERTIES COMPILE_FLAGS "-DUSING_CREG")
	set_target_properties(${KAIK_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	#set_target_properties(${KAIK_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "KAIK")
	install (TARGETS ${KAIK_TARGET} DESTINATION ${KAIK_DATA_DIR})
endif (BUILD_KAIK)

