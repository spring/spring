### NTai Skirmish AI
#
# BUILD_C_AIINTERFACE is set in ../../Interfaces/C/CMakeLists.txt
#

#set(NTai_SRC "${SKIRMISH_SRC}/NTai")
set(NTai_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if (BUILD_C_AIINTERFACE)
	set(BUILD_NTai TRUE)
else (BUILD_C_AIINTERFACE)
	set(BUILD_NTai FALSE)
	message ("warning: NTai Skirmish AI will not be built!")
endif (BUILD_C_AIINTERFACE)

if (BUILD_NTai)
	set(NTai_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NTai_SRC}/VERSION)
		file(STRINGS "${NTai_SRC}/VERSION" NTai_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NTai_SRC}/VERSION)
	#set(NTai_TARGET "NTai-${NTai_VERS}")
	set(NTai_TARGET "NTai")
	set(NTai_DATA_DIR ${SKIRMISH_DATA}/NTai/${NTai_VERS})
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NTai ${NTai_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NTai_SRC}/data/"
			DESTINATION "${NTai_DATA_DIR}")

	aux_source_directory(${NTai_SRC} ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Units ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Units/Behaviours ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Tasks ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers/grid ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers/Terrain ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Helpers/Units ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Engine ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Core ntai)
	aux_source_directory(${NTai_SRC}/AI/NTai/Agents ntai)
	add_library(${NTai_TARGET} MODULE ${ntai} ${cutilsenv})
	target_link_libraries(${NTai_TARGET} ${Boost_THREAD_LIBRARY} legacycpp)
	set_target_properties(${NTai_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	#set_target_properties(${NTai_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "NTai")
	install (TARGETS ${NTai_TARGET} DESTINATION ${NTai_DATA_DIR})
endif (BUILD_NTai)

