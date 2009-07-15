### NullOOJavaAI Skirmish AI
#
# BUILD_Java_AIINTERFACE is set in ../../Interfaces/Java/CMakeLists.txt
#

#set(NullOOJavaAI_SRC "${SKIRMISH_SRC}/NullOOJavaAI")
set(NullOOJavaAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if    (BUILD_Java_AIINTERFACE)
	set(BUILD_NullOOJavaAI TRUE)
else  (BUILD_Java_AIINTERFACE)
	set(BUILD_NullOOJavaAI FALSE)
	message ("warning: NullOOJavaAI Skirmish AI will not be built!")
endif (BUILD_Java_AIINTERFACE)

if    (BUILD_NullOOJavaAI)
	set(NullOOJavaAI_VERS "UNKNOWN_VERSION")
	if    (EXISTS ${NullOOJavaAI_SRC}/VERSION)
		file(STRINGS "${NullOOJavaAI_SRC}/VERSION" NullOOJavaAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullOOJavaAI_SRC}/VERSION)
	#set(NullOOJavaAI_TARGET "NullOOJavaAI-${NullOOJavaAI_VERS}")
	set(NullOOJavaAI_TARGET "NullOOJavaAI")
	set(NullOOJavaAI_DATA_DIR ${SKIRMISH_DATA}/NullOOJavaAI/${NullOOJavaAI_VERS})
	set(NullOOJavaAI_PKG "nulloojavaai")
	set(NullOOJavaAI_PKG_FIRST_PART "nulloojavaai")
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullOOJavaAI ${NullOOJavaAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullOOJavaAI_SRC}/data/"
			DESTINATION "${NullOOJavaAI_DATA_DIR}")

	# build the Java part
	# CMAKE_CURRENT_BINARY_DIR: .../spring-build-dir/AI/Skirmish/NullOOJavaAI
	set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	set(JAVA_BUILD_DIR "${BUILD_DIR}/classes")
	set(JAR_FILE "SkirmishAI")
	set(JAR_FILE_BIN "${JAR_FILE}.jar")
	set(JAR_FILE_SRC "${JAR_FILE}-src.jar")
	set(JAVA_SRC_DIR "${NullOOJavaAI_SRC}/src")
	set(JLIB_DIR "${NullOOJavaAI_SRC}/data/jlib")
	set(NullOOJavaAI_CLASSPATH "${CLASSPATH_Java_AIINTERFACE}")
	#set(NullOOJavaAI_CLASSPATH ".${PATH_DELIM}${CLASSPATH_Java_AIINTERFACE}${PATH_DELIM}${JLIB_DIR}/myOwnLib.jar")

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
endif (BUILD_NullOOJavaAI)

