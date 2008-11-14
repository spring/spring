### AI Interfaces

add_definitions(-DBUILDING_AI_INTERFACE)

set(AI_INTERFACES_SRC "${CMAKE_SOURCE_DIR}/AI/Interfaces")
set(AI_INTERFACES_DATA "${DATADIR}/AI/Interfaces")
IF (UNIX)
	set(SHELL_EXEC_POSTFIX "sh")
ELSE (UNIX)
	set(SHELL_EXEC_POSTFIX "bat")
ENDIF(UNIX)


# C & C++ AI Interface
set(C_AIINTERFACE_SRC "${AI_INTERFACES_SRC}/C")
if (EXISTS ${C_AIINTERFACE_SRC})
	set(C_AIINTERFACE_VERS "UNKNOWN_VERSION")
	if (EXISTS ${C_AIINTERFACE_SRC}/VERSION)
		file(STRINGS "${C_AIINTERFACE_SRC}/VERSION" C_AIINTERFACE_VERS LIMIT_COUNT 1)
	endif (EXISTS ${C_AIINTERFACE_SRC}/VERSION)
	set(C_AIINTERFACE_TARGET "C-${C_AIINTERFACE_VERS}")
	set(C_AIINTERFACE_DATA_DIR ${AI_INTERFACES_DATA}/C/${C_AIINTERFACE_VERS})
	message(STATUS "${C_AIINTERFACE_VERS}\t(C AI Interface Version)")

	aux_source_directory(${C_AIINTERFACE_SRC} cAIInterface)
	list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/SharedLib)
	if (UNIX)
		list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/SoLib)
	else (UNIX)
		list (APPEND cAIInterface ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/DllLib)
	endif (UNIX)
	add_library(${C_AIINTERFACE_TARGET} MODULE ${cAIInterface} ${aienv}) # ${errorhandler})
	install (FILES ${C_AIINTERFACE_SRC}/InterfaceInfo.lua DESTINATION ${C_AIINTERFACE_DATA_DIR})
	install (TARGETS ${C_AIINTERFACE_TARGET} DESTINATION ${C_AIINTERFACE_DATA_DIR})
endif (EXISTS ${C_AIINTERFACE_SRC})


# Java AI Interface
IF (MINGW)
	set (JAVA_JVM_LIBRARY jvm)
	set (JAVA_FOUND TRUE)
ELSE (MINGW)
	FIND_PACKAGE(JNI)
	if (JAVA_INCLUDE_PATH)
		include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} ${JNI_INCLUDE_DIRS})
		#link_directories(${JAVA_JVM_LIBRARY})
		#message("JAVA_JVM_LIBRARY ${JAVA_JVM_LIBRARY}")
		#message("JAVA_JVM_LIB_PATH ${JAVA_JVM_LIB_PATH}")
		#message("JNI_LIBRARIES ${JNI_LIBRARIES}")
		set (JAVA_FOUND TRUE)
	else (JAVA_INCLUDE_PATH)
		message ("No Java includes found -> Java AI Interface will not be built")
	endif (JAVA_INCLUDE_PATH)
ENDIF (MINGW)
set(Java_AIINTERFACE_SRC "${AI_INTERFACES_SRC}/Java")
if (JAVA_FOUND AND EXISTS ${Java_AIINTERFACE_SRC})
	set(Java_AIINTERFACE_VERS "UNKNOWN_VERSION")
	if (EXISTS ${Java_AIINTERFACE_SRC}/VERSION)
		file(STRINGS "${Java_AIINTERFACE_SRC}/VERSION" Java_AIINTERFACE_VERS LIMIT_COUNT 1)
	endif (EXISTS ${Java_AIINTERFACE_SRC}/VERSION)
	set(Java_AIINTERFACE_TARGET "Java-${Java_AIINTERFACE_VERS}")
	set(Java_AIINTERFACE_DATA_DIR ${AI_INTERFACES_DATA}/Java/${Java_AIINTERFACE_VERS})
	message(STATUS "${Java_AIINTERFACE_VERS}\t(Java AI Interface Version)")

	aux_source_directory(${Java_AIINTERFACE_SRC} javaAIInterface)
	add_library(${Java_AIINTERFACE_TARGET} MODULE ${javaAIInterface} ${aienv})
	target_link_libraries(${Java_AIINTERFACE_TARGET} ${JAVA_JVM_LIBRARY})
	install (FILES ${Java_AIINTERFACE_SRC}/InterfaceInfo.lua DESTINATION ${Java_AIINTERFACE_DATA_DIR})
	install (TARGETS ${Java_AIINTERFACE_TARGET} DESTINATION ${Java_AIINTERFACE_DATA_DIR})
	INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/rts/lib/streflop)
	TARGET_LINK_LIBRARIES(${Java_AIINTERFACE_TARGET} streflop)
	set_target_properties(${Java_AIINTERFACE_TARGET} PROPERTIES COMPILE_FLAGS "-DSTREFLOP_SSE")
	add_custom_command(TARGET ${Java_AIINTERFACE_TARGET} POST_BUILD
		COMMAND "${Java_AIINTERFACE_SRC}/bin/java_buildAll.${SHELL_EXEC_POSTFIX}"
		ARGS "${Java_AIINTERFACE_VERS}"
		WORKING_DIRECTORY "${Java_AIINTERFACE_SRC}/bin"
		COMMENT "  Creating Java library interface.jar" VERBATIM)

	# Install the Java part of the interface
	install (FILES ${Java_AIINTERFACE_SRC}/java/interface.jar DESTINATION ${Java_AIINTERFACE_DATA_DIR}/jlib)
	install(DIRECTORY ${Java_AIINTERFACE_SRC}/jlib/ DESTINATION ${Java_AIINTERFACE_DATA_DIR}/jlib FILES_MATCHING PATTERN "*.jar")

# Install the Java test Skirmish AI (only a temp solution here, move it to AI/Skirmish/...)
install (FILES ${Java_AIINTERFACE_SRC}/java/ai.jar DESTINATION ${CMAKE_SOURCE_DIR}/AI/Skirmish/hoijuiAI/${Java_AIINTERFACE_VERS})
install (FILES ${Java_AIINTERFACE_SRC}/AIInfo.lua DESTINATION ${CMAKE_SOURCE_DIR}/AI/Skirmish/hoijuiAI/${Java_AIINTERFACE_VERS})
endif (JAVA_FOUND AND EXISTS ${Java_AIINTERFACE_SRC})

