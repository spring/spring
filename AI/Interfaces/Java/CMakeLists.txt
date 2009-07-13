### Java AI Interface
#
# BUILD_Java_AIINTERFACE is set in this file (PARENT_SCOPE),
# and used in ../../Skirmish/*/CMakeLists.txt
#

if ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "JAVA")
	set(AIINTERFACES_JAVA "TRUE")
endif ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "JAVA")

if (NOT JAVA_FOUND)
	set(JAVA_FIND_QUIETLY ${AIINTERFACE_FIND_QUIETLY})
	FIND_PACKAGE(JavaExtended) # Note: this is not the CMake builtin FindJava
endif (NOT JAVA_FOUND)
IF (MINGW)
	set (JNI_FOUND TRUE)
ELSE (MINGW)
	# this hack is needed for FindJNI.cmake to use the JDK we want it to use,
	# as otherwise it might not find one at all (eg. in the case of OpenJDK)
	IF( NOT ENV{JAVA_HOME} AND JAVA_HOME )
		SET(ENV{JAVA_HOME} "${JAVA_HOME}")
	ENDIF( NOT ENV{JAVA_HOME} AND JAVA_HOME )
	FIND_PACKAGE(JNI)
	if (JAVA_INCLUDE_PATH)
		set (JNI_FOUND TRUE)
		include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} ${JNI_INCLUDE_DIRS})
	else (JAVA_INCLUDE_PATH)
		set (JNI_FOUND FALSE)
		message ("warning: No Java includes found!")
	endif (JAVA_INCLUDE_PATH)
ENDIF (MINGW)

#set(Java_AIINTERFACE_SRC "${AI_INTERFACES_SRC}/Java")
set(Java_AIINTERFACE_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

if (JNI_FOUND AND JAVA_FOUND AND EXISTS ${Java_AIINTERFACE_SRC} AND EXISTS ${Java_AIINTERFACE_SRC}/bin AND AIINTERFACES_JAVA)
	set(BUILD_Java_AIINTERFACE TRUE)
else (JNI_FOUND AND JAVA_FOUND AND EXISTS ${Java_AIINTERFACE_SRC} AND EXISTS ${Java_AIINTERFACE_SRC}/bin AND AIINTERFACES_JAVA)
	set(BUILD_Java_AIINTERFACE FALSE)
	message ("warning: Java AI Interface will not be built!")
endif (JNI_FOUND AND JAVA_FOUND AND EXISTS ${Java_AIINTERFACE_SRC} AND EXISTS ${Java_AIINTERFACE_SRC}/bin AND AIINTERFACES_JAVA)
set(BUILD_Java_AIINTERFACE ${BUILD_Java_AIINTERFACE} PARENT_SCOPE)

if (BUILD_Java_AIINTERFACE)
	set(Java_AIINTERFACE_VERS "UNKNOWN_VERSION")
	if (EXISTS ${Java_AIINTERFACE_SRC}/VERSION)
		file(STRINGS "${Java_AIINTERFACE_SRC}/VERSION" Java_AIINTERFACE_VERS LIMIT_COUNT 1)
	endif (EXISTS ${Java_AIINTERFACE_SRC}/VERSION)
	# The target has to be set in ../../Skirmish/CMakeLists.txt, aswell,
	# as compilation fails otherwise with multiple threads,
	# using PARENT_SCOPE seems not to work.
	#set(Java_AIINTERFACE_TARGET "Java-${Java_AIINTERFACE_VERS}")
	set(Java_AIINTERFACE_TARGET "Java-AI-Interface")
	#set(Java_AIINTERFACE_TARGET ${Java_AIINTERFACE_TARGET} PARENT_SCOPE)
	set(Java_AIINTERFACE_DATA_DIR ${AI_INTERFACES_DATA}/Java/${Java_AIINTERFACE_VERS})
	set(Java_AIINTERFACE_PKG "com/springrts/ai")
	set(PKG_FIRST_PART "com")
	if (NOT AIINTERFACE_FIND_QUIETLY)
		message(STATUS "Found: ${Java_AIINTERFACE_TARGET} ${Java_AIINTERFACE_VERS}")
	endif (NOT AIINTERFACE_FIND_QUIETLY)

	# install data files
	install(DIRECTORY "${Java_AIINTERFACE_SRC}/data/"
			DESTINATION "${Java_AIINTERFACE_DATA_DIR}")

	# build the native part
	aux_source_directory(${Java_AIINTERFACE_SRC} javaAIInterface)
	IF (MINGW)
		# It is important that this is used instead of the one
		# from the installed JDK, as the jni_md.h is in here too,
		# and this file contians OS (win32) specific information.
		INCLUDE_DIRECTORIES(BEFORE ${MINGWLIBS}/include/java)
	ENDIF(MINGW)
	INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/rts/lib/streflop)
	add_library(${Java_AIINTERFACE_TARGET} MODULE ${javaAIInterface} ${ai_common} ${cutilsenv})
	install(TARGETS ${Java_AIINTERFACE_TARGET} DESTINATION ${Java_AIINTERFACE_DATA_DIR})
	TARGET_LINK_LIBRARIES(${Java_AIINTERFACE_TARGET} streflop)
	set_target_properties(${Java_AIINTERFACE_TARGET} PROPERTIES COMPILE_FLAGS "-DUSING_STREFLOP")
	set_target_properties(${Java_AIINTERFACE_TARGET} PROPERTIES OUTPUT_NAME "AIInterface")
	#set_target_properties(${Java_AIINTERFACE_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Java")

	# build the Java part
	IF (CMAKE_HOST_WIN32)
		set(AWK_COMMAND "${MINGWLIBS}/bin/awk.exe")
	ELSE (CMAKE_HOST_WIN32)
		set(AWK_COMMAND "awk")
	ENDIF(CMAKE_HOST_WIN32)
	set(AWK_SCRIPTS_DIR "${Java_AIINTERFACE_SRC}/bin")
	set(AWK_COMMON_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/Java")
	set(SPRING_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
	set(SPRING_AIINTERFACE_SOURCE_DIR "${SPRING_SOURCE_DIR}/rts/ExternalAI/Interface")
	set(JAVA_BUILD_DIR "${BUILD_DIR}/classes")
	set(JAR_FILE "AIInterface")
	set(JAR_FILE_BIN "${JAR_FILE}.jar")
	set(JAR_FILE_SRC "${JAR_FILE}-src.jar")
	set(JAVA_SRC_DIR "${Java_AIINTERFACE_SRC}/java/src")
	set(JAVA_GENERATED_SRC_DIR "${BUILD_DIR}/generated-java-src")
	set(JLIB_DIR "${Java_AIINTERFACE_SRC}/data/jlib")
	set(CLASSPATH_LIBS "${JLIB_DIR}/jna/jna.jar${PATH_DELIM}${JLIB_DIR}/vecmath.jar")
	set(CLASSPATH_INTERNAL ".${PATH_DELIM}${CLASSPATH_LIBS}${PATH_DELIM}${JAVA_SRC_DIR}")

	# Used by Java Skirmish AIs
	set(CLASSPATH_Java_AIINTERFACE "${CLASSPATH_LIBS}${PATH_DELIM}${BUILD_DIR}/${JAR_FILE_BIN}" PARENT_SCOPE)
	#set(JAR_FILE_BIN_AIINTERFACE "${BUILD_DIR}/${JAR_FILE_BIN}" PARENT_SCOPE)

	# We have to create these directories, cause otherwise the AWK scripts
	# for generating the source code will not work
	file(MAKE_DIRECTORY "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/event")
	file(MAKE_DIRECTORY "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/command")
	file(MAKE_DIRECTORY "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo")
	# needed by javac
	file(MAKE_DIRECTORY "${JAVA_BUILD_DIR}")

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#OUTPUT "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/event/*.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "SPRING_SOURCE_DIR=${SPRING_SOURCE_DIR}"
		"-v" "INTERFACE_SOURCE_DIR=${JAVA_SRC_DIR}"
		"-v" "GENERATED_SOURCE_DIR=${JAVA_GENERATED_SRC_DIR}"
		"-f" "${AWK_SCRIPTS_DIR}/jna_wrappEvents.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/common.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonDoc.awk"
		"${SPRING_AIINTERFACE_SOURCE_DIR}/AISEvents.h"
		WORKING_DIRECTORY "${AWK_SCRIPTS_DIR}"
		COMMENT "  Generating Java AI Event source files ..." VERBATIM)

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#OUTPUT "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/command/*.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "SPRING_SOURCE_DIR=${SPRING_SOURCE_DIR}"
		"-v" "INTERFACE_SOURCE_DIR=${JAVA_SRC_DIR}"
		"-v" "GENERATED_SOURCE_DIR=${JAVA_GENERATED_SRC_DIR}"
		"-f" "${AWK_SCRIPTS_DIR}/jna_wrappCommands.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/common.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonDoc.awk"
		"${SPRING_AIINTERFACE_SOURCE_DIR}/AISCommands.h"
		WORKING_DIRECTORY "${AWK_SCRIPTS_DIR}"
		COMMENT "  Generating Java AI Command source files ..." VERBATIM)

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#OUTPUT "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/AICallback.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "SPRING_SOURCE_DIR=${SPRING_SOURCE_DIR}"
		"-v" "INTERFACE_SOURCE_DIR=${JAVA_SRC_DIR}"
		"-v" "GENERATED_SOURCE_DIR=${JAVA_GENERATED_SRC_DIR}"
		"-f" "${AWK_SCRIPTS_DIR}/jna_wrappCallback.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/common.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonDoc.awk"
		"${SPRING_AIINTERFACE_SOURCE_DIR}/SSkirmishAICallback.h"
		WORKING_DIRECTORY "${AWK_SCRIPTS_DIR}"
		COMMENT "  Generating Java AI Callback source files ..." VERBATIM)

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/AICallback.java"
		#OUTPUT "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo/*.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "SPRING_SOURCE_DIR=${SPRING_SOURCE_DIR}"
		"-v" "INTERFACE_SOURCE_DIR=${JAVA_SRC_DIR}"
		"-v" "GENERATED_SOURCE_DIR=${JAVA_GENERATED_SRC_DIR}"
		"-f" "${AWK_SCRIPTS_DIR}/java_wrappCallbackOO.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/common.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonDoc.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonOOCallback.awk"
		"${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/AICallback.java"
		WORKING_DIRECTORY "${AWK_SCRIPTS_DIR}"
		COMMENT "  Generating Java AI OO Callback source files ..." VERBATIM)

	# We can not auto generate this list,
	# as these files do not yet exist during configure stage
	# The files in this list have to depend on all other hand-written
	# and generated source files of the Java interface
	set(JAVA_GENERATED_SOURCES
		"${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo/OOAICallback.java"
		"${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo/OOAIFactory.java"
		"${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo/AbstractOOAI.java")

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#OUTPUT "${JAVA_BUILD_DIR}/*.class"
		DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/*.java"
		DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/event/*.java"
		DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/command/*.java"
		DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo/*.java"
		COMMAND "${JAVA_COMPILE}" ARGS
		"${JAVA_COMPILE_FLAG_CONDITIONAL}"
		"-cp" "${CLASSPATH_INTERNAL}"
		"-d" "${JAVA_BUILD_DIR}"
		${JAVA_GENERATED_SOURCES}
		WORKING_DIRECTORY "${JAVA_GENERATED_SRC_DIR}"
		COMMENT "  Compiling Java sources ..." VERBATIM)

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#DEPENDS "${JAVA_BUILD_DIR}/*.class"
		COMMAND "${JAVA_ARCHIVE}" ARGS
		"cmf" "${JAVA_SRC_DIR}/manifest.mf" "${BUILD_DIR}/${JAR_FILE_BIN}"
		"-C" "${JAVA_BUILD_DIR}" "${PKG_FIRST_PART}"
		WORKING_DIRECTORY "${BUILD_DIR}"
		COMMENT "  Creating Java interface library ${JAR_FILE_BIN} ..." VERBATIM)

	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		COMMAND "${JAVA_ARCHIVE}" ARGS
		"cf" "${BUILD_DIR}/${JAR_FILE_SRC}"
		"-C" "${JAVA_SRC_DIR}" "${PKG_FIRST_PART}"
		WORKING_DIRECTORY "${BUILD_DIR}"
		COMMENT "  Creating Java interface source archive ${JAR_FILE_SRC} ..." VERBATIM)
	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		#DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/event/*.java"
		#DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/command/*.java"
		#DEPENDS "${JAVA_GENERATED_SRC_DIR}/${Java_AIINTERFACE_PKG}/oo/*.java"
		COMMAND "${JAVA_ARCHIVE}" ARGS
		"uf" "${BUILD_DIR}/${JAR_FILE_SRC}"
		"-C" "${JAVA_GENERATED_SRC_DIR}" "${PKG_FIRST_PART}"
		WORKING_DIRECTORY "${BUILD_DIR}"
		COMMENT "  Updating Java interface source archive ${JAR_FILE_SRC} with generated sources ..." VERBATIM)

	install(FILES ${BUILD_DIR}/${JAR_FILE_BIN} DESTINATION ${Java_AIINTERFACE_DATA_DIR})
	install(FILES ${BUILD_DIR}/${JAR_FILE_SRC} DESTINATION ${Java_AIINTERFACE_DATA_DIR}/jlib)
endif (BUILD_Java_AIINTERFACE)

