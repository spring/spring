### NullJavaAI Skirmish AI
#
# BUILD_Java_AIINTERFACE is set in ../../Interfaces/Java/CMakeLists.txt
#

#set(NullJavaAI_SRC "${SKIRMISH_SRC}/NullJavaAI")
set(NullJavaAI_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if    (BUILD_Java_AIINTERFACE)
	set(BUILD_NullJavaAI TRUE)
else  (BUILD_Java_AIINTERFACE)
	set(BUILD_NullJavaAI FALSE)
	message ("warning: NullJavaAI Skirmish AI will not be built!")
endif (BUILD_Java_AIINTERFACE)

if    (BUILD_NullJavaAI)
	set(NullJavaAI_VERS "UNKNOWN_VERSION")
	if    (EXISTS ${NullJavaAI_SRC}/VERSION)
		file(STRINGS "${NullJavaAI_SRC}/VERSION" NullJavaAI_VERS LIMIT_COUNT 1)
	endif (EXISTS ${NullJavaAI_SRC}/VERSION)
	#set(NullJavaAI_TARGET "NullJavaAI-${NullJavaAI_VERS}")
	set(NullJavaAI_TARGET "NullJavaAI")
	set(NullJavaAI_DATA_DIR ${SKIRMISH_DATA}/NullJavaAI/${NullJavaAI_VERS})
	set(NullJavaAI_PKG "nulljavaai")
	set(NullJavaAI_PKG_FIRST_PART "nulljavaai")
	if (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "Found Skirmish AI: NullJavaAI ${NullJavaAI_VERS}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${NullJavaAI_SRC}/data/"
			DESTINATION "${NullJavaAI_DATA_DIR}")

	# build the Java part
	# CMAKE_CURRENT_BINARY_DIR: .../spring-build-dir/AI/Skirmish/NullJavaAI
	set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	set(JAVA_BUILD_DIR "${BUILD_DIR}/classes")
	set(JAR_FILE "SkirmishAI")
	set(JAR_FILE_BIN "${JAR_FILE}.jar")
	set(JAR_FILE_SRC "${JAR_FILE}-src.jar")
	set(JAVA_SRC_DIR "${NullJavaAI_SRC}/src")
	set(JLIB_DIR "${NullJavaAI_SRC}/data/jlib")
	set(NullJavaAI_CLASSPATH "${CLASSPATH_Java_AIINTERFACE}")
	#set(NullJavaAI_CLASSPATH ".${PATH_DELIM}${CLASSPATH_Java_AIINTERFACE}${PATH_DELIM}${JLIB_DIR}/myOwnLib.jar")

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
endif (BUILD_NullJavaAI)

