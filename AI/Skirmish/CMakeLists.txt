### Skirmish AIs
#
# BUILD_C_AIINTERFACE and BUILD_Java_AIINTERFACE
# are set in ../Interfaces/CMakeLists.txt
#

add_definitions(-DBUILDING_SKIRMISH_AI)

set(SKIRMISH_SRC "${CMAKE_SOURCE_DIR}/AI/Skirmish")
set(SKIRMISH_DATA "${AIDIR}/AI/Skirmish")

# These seem to be needed when compiling with multiple threads.
# They are really set in ../Interfaces/CMakeLists.txt.
set(C_AIINTERFACE_TARGET    "C-AI-Interface")
set(Java_AIINTERFACE_TARGET "Java-AI-Interface")

if (BUILD_Java_AIINTERFACE)
	set(JAVA_FIND_QUIETLY ${AIINTERFACE_FIND_QUIETLY})
	FIND_PACKAGE(JavaExtended) # Note: this is not the CMake builtin FindJava
endif (BUILD_Java_AIINTERFACE)

#set(SKIRMISHAI_FIND_QUIETLY True)


## KAIK
set(KAIK_SRC "${SKIRMISH_SRC}/KAIK")
if (BUILD_C_AIINTERFACE AND EXISTS ${KAIK_SRC})
	set(KAIK_VERS "UNKNOWN_VERSION")
	if (EXISTS ${KAIK_SRC}/VERSION)
		file(STRINGS "${KAIK_SRC}/VERSION" KAIK_VERS LIMIT_COUNT 1)
	endif (EXISTS ${KAIK_SRC}/VERSION)
	#set(KAIK_TARGET "KAIK-${KAIK_VERS}")
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
	set_target_properties(${KAIK_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "KAIK")
	install (TARGETS ${KAIK_TARGET} DESTINATION ${KAIK_DATA_DIR})
endif (BUILD_C_AIINTERFACE AND EXISTS ${KAIK_SRC})


## RAI
set(RAI_SRC "${SKIRMISH_SRC}/RAI")
if (BUILD_C_AIINTERFACE AND EXISTS ${RAI_SRC})
	set(RAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${RAI_SRC}/VERSION)
		file(STRINGS "${RAI_SRC}/VERSION" RAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${RAI_SRC}/VERSION)
	#set(RAI_TARGET "RAI-${RAI_VERS}")
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
	set_target_properties(${RAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "RAI")
	install (TARGETS ${RAI_TARGET} DESTINATION ${RAI_DATA_DIR})
endif (BUILD_C_AIINTERFACE AND EXISTS ${RAI_SRC})


## AAI
set(AAI_SRC "${SKIRMISH_SRC}/AAI")
if (BUILD_C_AIINTERFACE AND EXISTS ${AAI_SRC})
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
	set_target_properties(${AAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "AAI")
	install (TARGETS ${AAI_TARGET} DESTINATION ${AAI_DATA_DIR})
endif (BUILD_C_AIINTERFACE AND EXISTS ${AAI_SRC})


## NTai
set(NTai_SRC "${SKIRMISH_SRC}/NTai")
if (BUILD_C_AIINTERFACE AND EXISTS ${NTai_SRC})
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
	set_target_properties(${NTai_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "NTai")
	install (TARGETS ${NTai_TARGET} DESTINATION ${NTai_DATA_DIR})
endif (BUILD_C_AIINTERFACE AND EXISTS ${NTai_SRC})


## Null Legacy C++ AI
set(NullLegacyCppAI_SRC "${SKIRMISH_SRC}/NullLegacyCppAI")
if (BUILD_C_AIINTERFACE AND EXISTS ${NullLegacyCppAI_SRC})
	set(NullLegacyCppAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NullLegacyCppAI_SRC}/VERSION)
		file(STRINGS "${NullLegacyCppAI_SRC}/VERSION" NullLegacyCppAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullLegacyCppAI_SRC}/VERSION)
	#set(NullLegacyCppAI_TARGET "NullLegacyCppAI-${NullLegacyCppAI_VERS}")
	set(NullLegacyCppAI_TARGET "NullLegacyCppAI")
	set(NullLegacyCppAI_DATA_DIR ${SKIRMISH_DATA}/NullLegacyCppAI/${NullLegacyCppAI_VERS})
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullLegacyCppAI ${NullLegacyCppAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullLegacyCppAI_SRC}/data/"
			DESTINATION "${NullLegacyCppAI_DATA_DIR}")

	aux_source_directory(${NullLegacyCppAI_SRC} nulllegacycppai)
	add_library(${NullLegacyCppAI_TARGET} MODULE ${nulllegacycppai} ${cutilsenv})
	target_link_libraries(${NullLegacyCppAI_TARGET} legacycpp)
	set_target_properties(${NullLegacyCppAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	set_target_properties(${NullLegacyCppAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "NullLegacyCppAI")
	install (TARGETS ${NullLegacyCppAI_TARGET} DESTINATION ${NullLegacyCppAI_DATA_DIR})
endif (BUILD_C_AIINTERFACE AND EXISTS ${NullLegacyCppAI_SRC})


## Null C AI
set(NullAI_SRC "${SKIRMISH_SRC}/NullAI")
if (BUILD_C_AIINTERFACE AND EXISTS ${NullAI_SRC})
	set(NullAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NullAI_SRC}/VERSION)
		file(STRINGS "${NullAI_SRC}/VERSION" NullAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullAI_SRC}/VERSION)
	#set(NullAI_TARGET "NullAI-${NullAI_VERS}")
	set(NullAI_TARGET "NullAI")
	set(NullAI_DATA_DIR ${SKIRMISH_DATA}/NullAI/${NullAI_VERS})
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullAI ${NullAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullAI_SRC}/data/"
			DESTINATION "${NullAI_DATA_DIR}")

	aux_source_directory(${NullAI_SRC} nullai)
	add_library(${NullAI_TARGET} MODULE ${mySrc} ${nullai})
	set_target_properties(${NullAI_TARGET} PROPERTIES OUTPUT_NAME "SkirmishAI")
	set_target_properties(${NullAI_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "NullAI")
	install (TARGETS ${NullAI_TARGET} DESTINATION ${NullAI_DATA_DIR})
endif (BUILD_C_AIINTERFACE AND EXISTS ${NullAI_SRC})


## Null Java AI
set(NullJavaAI_SRC "${SKIRMISH_SRC}/NullJavaAI")
if (BUILD_Java_AIINTERFACE AND EXISTS ${NullJavaAI_SRC})
	set(NullJavaAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NullJavaAI_SRC}/VERSION)
		file(STRINGS "${NullJavaAI_SRC}/VERSION" NullJavaAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullJavaAI_SRC}/VERSION)
	#set(NullJavaAI_TARGET "NullJavaAI-${NullJavaAI_VERS}")
	set(NullJavaAI_TARGET "NullJavaAI")
	set(NullJavaAI_DATA_DIR ${SKIRMISH_DATA}/NullJavaAI/${NullJavaAI_VERS})
	set(NullJavaAI_PKG "nulljavaai")
	set(NullJavaAI_PKG_FIRST_PART "nulljavaai")
	set(Java_AIINTERFACE_SRC "../../Interfaces/Java")
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullJavaAI ${NullJavaAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullJavaAI_SRC}/data/"
			DESTINATION "${NullJavaAI_DATA_DIR}")

	# build the Java part
	# CMAKE_CURRENT_BINARY_DIR: .../spring-build-dir/AI/Skirmish
	set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/NullJavaAI")
	set(JAVA_BUILD_DIR "${BUILD_DIR}/classes")
	set(JAR_FILE "SkirmishAI")
	set(JAR_FILE_BIN "${JAR_FILE}.jar")
	set(JAR_FILE_SRC "${JAR_FILE}-src.jar")
	set(JAVA_SRC_DIR "${NullJavaAI_SRC}/src")
	set(JLIB_DIR "${NullJavaAI_SRC}/data/jlib")
	set(NullJavaAI_CLASSPATH "${CLASSPATH_AIINTERFACE}")
	#set(NullJavaAI_CLASSPATH ".${PATH_DELIM}${CLASSPATH_AIINTERFACE}${PATH_DELIM}${JLIB_DIR}/myOwnLib.jar")

	# needed by javac
	file(MAKE_DIRECTORY "${JAVA_BUILD_DIR}")

	file(GLOB_RECURSE NullJavaAI_JAVA_SOURCES RELATIVE "${JAVA_SRC_DIR}" FOLLOW_SYMLINKS "${JAVA_SRC_DIR}/*.java")
	#add_custom_command(OUTPUT "${NullJavaAI_DATA_DIR}/${JAR_FILE_BIN}"
	#add_custom_command(TARGET ${NullJavaAI_TARGET}
	add_custom_target(${NullJavaAI_TARGET} ALL
		DEPENDS ${Java_AIINTERFACE_TARGET}
		COMMAND "${JAVA_COMPILE}"
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-cp" "${NullJavaAI_CLASSPATH}"
			"-d" "${JAVA_BUILD_DIR}"
			${NullJavaAI_JAVA_SOURCES}
		COMMAND "${JAVA_ARCHIVE}"
			"cmf" "${NullJavaAI_SRC}/manifest.mf" "${BUILD_DIR}/${JAR_FILE_BIN}"
			"-C" "${JAVA_BUILD_DIR}" "${NullJavaAI_PKG_FIRST_PART}"
		WORKING_DIRECTORY "${JAVA_SRC_DIR}"
		COMMENT "  Creating Java AI library ${JAR_FILE_BIN}" VERBATIM)
	add_custom_target(${NullJavaAI_TARGET}-SRC ALL
		COMMAND "${JAVA_ARCHIVE}"
			"cf" "${BUILD_DIR}/${JAR_FILE_SRC}"
			"-C" "${JAVA_SRC_DIR}" "${NullJavaAI_PKG_FIRST_PART}"
		WORKING_DIRECTORY "${JAVA_SRC_DIR}"
		COMMENT "  Creating Java AI library ${JAR_FILE_SRC}" VERBATIM)

	install (FILES ${BUILD_DIR}/${JAR_FILE_BIN} DESTINATION ${NullJavaAI_DATA_DIR})
	install (FILES ${BUILD_DIR}/${JAR_FILE_SRC} DESTINATION ${NullJavaAI_DATA_DIR}/jlib)
endif (BUILD_Java_AIINTERFACE AND EXISTS ${NullJavaAI_SRC})


## Null Object Oriented Java AI
set(NullOOJavaAI_SRC "${SKIRMISH_SRC}/NullOOJavaAI")
if (BUILD_Java_AIINTERFACE AND EXISTS ${NullOOJavaAI_SRC})
	set(NullOOJavaAI_VERS "UNKNOWN_VERSION")
	if (EXISTS ${NullOOJavaAI_SRC}/VERSION)
		file(STRINGS "${NullOOJavaAI_SRC}/VERSION" NullOOJavaAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullOOJavaAI_SRC}/VERSION)
	#set(NullOOJavaAI_TARGET "NullOOJavaAI-${NullOOJavaAI_VERS}")
	set(NullOOJavaAI_TARGET "NullOOJavaAI")
	set(NullOOJavaAI_DATA_DIR ${SKIRMISH_DATA}/NullOOJavaAI/${NullOOJavaAI_VERS})
	set(NullOOJavaAI_PKG "nulloojavaai")
	set(NullOOJavaAI_PKG_FIRST_PART "nulloojavaai")
	set(Java_AIINTERFACE_SRC "../../Interfaces/Java")
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullOOJavaAI ${NullOOJavaAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullOOJavaAI_SRC}/data/"
			DESTINATION "${NullOOJavaAI_DATA_DIR}")

	# build the Java part
	# CMAKE_CURRENT_BINARY_DIR: .../spring-build-dir/AI/Skirmish
	set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/NullOOJavaAI")
	set(JAVA_BUILD_DIR "${BUILD_DIR}/classes")
	set(JAR_FILE "SkirmishAI")
	set(JAR_FILE_BIN "${JAR_FILE}.jar")
	set(JAR_FILE_SRC "${JAR_FILE}-src.jar")
	set(JAVA_SRC_DIR "${NullOOJavaAI_SRC}/src")
	set(JLIB_DIR "${NullOOJavaAI_SRC}/data/jlib")
	set(NullOOJavaAI_CLASSPATH "${CLASSPATH_AIINTERFACE}")
	#set(NullOOJavaAI_CLASSPATH ".${PATH_DELIM}${CLASSPATH_AIINTERFACE}${PATH_DELIM}${JLIB_DIR}/myOwnLib.jar")

	# needed by javac
	file(MAKE_DIRECTORY "${JAVA_BUILD_DIR}")

	file(GLOB_RECURSE NullOOJavaAI_JAVA_SOURCES RELATIVE "${JAVA_SRC_DIR}" FOLLOW_SYMLINKS "${JAVA_SRC_DIR}/*.java")
	#add_custom_command(OUTPUT "${NullOOJavaAI_DATA_DIR}/${JAR_FILE_BIN}"
	#add_custom_command(TARGET ${NullOOJavaAI_TARGET}
	add_custom_target(${NullOOJavaAI_TARGET} ALL
		DEPENDS ${Java_AIINTERFACE_TARGET}
		COMMAND "${JAVA_COMPILE}"
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-cp" "${NullOOJavaAI_CLASSPATH}"
			"-d" "${JAVA_BUILD_DIR}"
			${NullOOJavaAI_JAVA_SOURCES}
		COMMAND "${JAVA_ARCHIVE}"
			"cmf" "${NullOOJavaAI_SRC}/manifest.mf" "${BUILD_DIR}/${JAR_FILE_BIN}"
			"-C" "${JAVA_BUILD_DIR}" "${NullOOJavaAI_PKG_FIRST_PART}"
		WORKING_DIRECTORY "${JAVA_SRC_DIR}"
		COMMENT "  Creating Java AI library ${JAR_FILE_BIN}" VERBATIM)
	add_custom_target(${NullOOJavaAI_TARGET}-SRC ALL
		COMMAND "${JAVA_ARCHIVE}"
			"cf" "${BUILD_DIR}/${JAR_FILE_SRC}"
			"-C" "${JAVA_SRC_DIR}" "${NullOOJavaAI_PKG_FIRST_PART}"
		WORKING_DIRECTORY "${JAVA_SRC_DIR}"
		COMMENT "  Creating Java AI library ${JAR_FILE_SRC}" VERBATIM)

	install (FILES ${BUILD_DIR}/${JAR_FILE_BIN} DESTINATION ${NullOOJavaAI_DATA_DIR})
	install (FILES ${BUILD_DIR}/${JAR_FILE_SRC} DESTINATION ${NullOOJavaAI_DATA_DIR}/jlib)
endif (BUILD_Java_AIINTERFACE AND EXISTS ${NullOOJavaAI_SRC})
