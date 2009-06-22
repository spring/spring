################################################################################
# cmake module for finding Java
# The name JavaExtended is used instead of JAVA, as this cause an infinite loop
# on windows, because on windows, there is no difference between the built-in
# FindJava.cmake and FindJAVA.cmake.
#
# sets following variables:
#
# JAVA_FOUND    - TRUE/FALSE (javadoc in ignored for the test)
#
# JAVA_HOME     - base dir where subdirs bin, lib, jre and include are located
# JAVA_BIN_PATH - path to subdirectory "${JAVA_HOME}/bin"
#
# JAVA_RUNTIME  - path to ${JAVA_BIN_PATH}/java
# JAVA_COMPILE  - path to ${JAVA_BIN_PATH}/javac
# JAVA_ARCHIVE  - path to ${JAVA_BIN_PATH}/jar
# JAVA_DOC      - path to ${JAVA_BIN_PATH}/javadoc
#
# JAVA_COMPILE_FLAG_CONDITIONAL      - "-g:lines,source" or "-g:lines,source,vars"
#
# JAVA_MAJOR_VERSION
# JAVA_MINOR_VERSION
# JAVA_PATCH_LEVEL
#
# JAVA_VERSION - ${JAVA_MAJOR_VERSION}.${JAVA_MINOR_VERSION}.${JAVA_PATCH_LEVEL}
#
# @author Jan Engels, DESY, hoijui
################################################################################

SET( JAVA_FOUND FALSE )
MARK_AS_ADVANCED( JAVA_FOUND )

IF ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
	set(JAVA_COMPILE_FLAG_CONDITIONAL "-g:lines,source")
ELSE ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
	set(JAVA_COMPILE_FLAG_CONDITIONAL "-g:lines,source,vars")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")

IF( NOT DEFINED ENV{JDK_HOME} AND 
	NOT DEFINED ENV{JAVA_HOME} AND
	NOT DEFINED JAVA_HOME AND
	NOT DEFINED JDK_HOME )

	IF( NOT JAVA_FIND_QUIETLY )
		MESSAGE( STATUS "Autodetecting Java..." )
	ENDIF()

	# use the CMake FindJava.cmake module
	FIND_PACKAGE( Java )

	IF( JAVA_RUNTIME AND JAVA_COMPILE AND JAVA_ARCHIVE )
		IF( CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE )
			# look for the file binary
			FIND_PROGRAM( FILE_BIN
				file
				/bin
				/usr/bin
				/usr/local/bin
				/sbin
			)
			MARK_AS_ADVANCED( FILE_BIN )

			IF( FILE_BIN )

				# initialize some variables
				SET( java_bin "${JAVA_RUNTIME}" )
				SET( java_link_found TRUE )
				SET( java_link_error FALSE )
				MARK_AS_ADVANCED( java_bin java_link_found java_link_error )

				# dereference links
				WHILE( java_link_found AND NOT java_link_error )
					# check if the java binary is a symbolic link
					EXEC_PROGRAM( ${FILE_BIN} ARGS "${java_bin}"
						OUTPUT_VARIABLE out_tmp
						RETURN_VALUE out_ret )
					IF( out_ret )
						IF( NOT JAVA_FIND_QUIETLY )
							MESSAGE( STATUS "Error dereferencing links to Java Home!!" )
							MESSAGE( STATUS "${out_tmp}" )
						ENDIF()
						SET( java_link_error TRUE )
					ENDIF()
					IF( NOT java_link_error )
						# set variable if link is found
						STRING( REGEX MATCH " symbolic link to " java_link_found "${out_tmp}" )
						IF( java_link_found )
							# get the file to where the link points to
							STRING( REGEX REPLACE ".* symbolic link to[^/.]+([^'`´]+).*" "\\1" out_regex "${out_tmp}" )
							# check if the link is absolute
							STRING( REGEX REPLACE "^\\/.*$" "true" out_regex_is_absolute "${out_regex}" )
							IF( "${out_regex_is_absolute}" STREQUAL "true" )
								SET( out_absolute "${out_regex}" )
							ELSE( "${out_regex_is_absolute}" STREQUAL "true" )
								# get the links parent dir
								STRING( REGEX REPLACE "(.*)\\/[^/]+$" "\\1" java_bin_dir "${java_bin}" )
								# make the link absolute, if it is relative
								FIND_FILE( out_absolute NAMES "${out_regex}" PATHS "${java_bin_dir}" )
							ENDIF( "${out_regex_is_absolute}" STREQUAL "true" )
							IF( NOT JAVA_FIND_QUIETLY )
								MESSAGE( STATUS "Java binary ${java_bin} is a symbolic link to ${out_regex}" )
							ENDIF()
							SET( java_bin "${out_absolute}" )
						ENDIF()
					ENDIF()
				ENDWHILE()

				# check if something went wrong
				IF( NOT java_link_error )
					# if not set JAVA_RUNTIME to the dereferenced link
					SET( JAVA_RUNTIME "${java_bin}" )
				ENDIF()
			ENDIF()
		ENDIF( CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE )

		IF( NOT JDK_HOME )
			# extract java home dir name out of full path to java runtime
			STRING( REGEX REPLACE ".*\\/([^/]+)\\/[^/]+\\/java$" "\\1" java_home_dir_name ${JAVA_RUNTIME} )
			# check if we are in a child jre
			STRING( COMPARE EQUAL "${java_home_dir_name}" "jre" java_child_jre )
			IF( ${java_child_jre} )
				# extract jdk home path out of full path to java runtime
				STRING( REGEX REPLACE "(.*)\\/[^/]+\\/[^/]+\\/java$" "\\1" JDK_HOME ${JAVA_RUNTIME} )
			ENDIF( ${java_child_jre} )
		ENDIF( NOT JDK_HOME )
		IF( NOT JAVA_HOME )
			IF( JDK_HOME )
				SET( JAVA_HOME "${JDK_HOME}" )
			ELSE( JDK_HOME )
				# extract java home path out of full path to java runtime
				STRING( REGEX REPLACE "(.*)\\/[^/]+\\/java$" "\\1" JAVA_HOME ${JAVA_RUNTIME} )
			ENDIF( JDK_HOME )
		ENDIF( NOT JAVA_HOME )
		# extract java bin path out of full path to java runtime
		STRING( REGEX REPLACE "(.*)\\/java$" "\\1" JAVA_BIN_PATH ${JAVA_RUNTIME} )
		#SET( JAVA_BIN_PATH "${JAVA_HOME}/bin" )
	ELSE()
		IF( NOT JAVA_FIND_QUIETLY )
			MESSAGE( STATUS "Failed to autodetect Java!!" )
		ENDIF()
	ENDIF( JAVA_RUNTIME AND JAVA_COMPILE AND JAVA_ARCHIVE )
ELSE()
	# definition of JAVA_HOME or JDK_HOME in cmake has priority over env vars
	IF( DEFINED JDK_HOME OR DEFINED JAVA_HOME )
		# ensure that both variables are set correctly (JDK_HOME as well as JAVA_HOME)
		IF( DEFINED JDK_HOME AND DEFINED JAVA_HOME )
			IF( NOT "${JDK_HOME}" STREQUAL "${JAVA_HOME}" )
				IF( NOT JAVA_FIND_QUIETLY )
					MESSAGE( STATUS
						"WARNING: JDK_HOME and JAVA_HOME are set to different paths!!" )
					MESSAGE( STATUS "JDK_HOME: ${JDK_HOME}" )
					MESSAGE( STATUS "JAVA_HOME: ${JAVA_HOME}" )
					MESSAGE( STATUS "${JAVA_HOME} will be used in this installation!!" )
				ENDIF()
			ENDIF()
		ELSE()
			IF( NOT DEFINED JAVA_HOME )
				SET( JAVA_HOME "${JDK_HOME}" )
			ENDIF()
		ENDIF()
	ELSE()
		# in case JDK_HOME or JAVA_HOME already set ensure that both variables
		# are set correctly (JDK_HOME as well as JAVA_HOME)
		IF( DEFINED ENV{JDK_HOME} AND DEFINED ENV{JAVA_HOME} )
			IF( NOT "$ENV{JDK_HOME}" STREQUAL "$ENV{JAVA_HOME}" )
				IF( NOT JAVA_FIND_QUIETLY )
					MESSAGE( STATUS
						"WARNING: JDK_HOME and JAVA_HOME are set to different paths!!" )
					MESSAGE( STATUS "JDK_HOME: $ENV{JDK_HOME}" )
					MESSAGE( STATUS "JAVA_HOME: $ENV{JAVA_HOME}" )
					MESSAGE( STATUS "$ENV{JAVA_HOME} will be used in this installation!!" )
				ENDIF()
			ENDIF()
			SET( JAVA_HOME "$ENV{JAVA_HOME}" )
		ELSE()
			IF( DEFINED ENV{JAVA_HOME} )
				SET( JAVA_HOME "$ENV{JAVA_HOME}" )
			ENDIF()
			IF( DEFINED ENV{JDK_HOME} )
				SET( JAVA_HOME "$ENV{JDK_HOME}" )
			ENDIF()
		ENDIF()
	ENDIF()

	set(JAVA_BINSEARCH_PATH
		${JAVA_HOME}/bin
		${JAVA_HOME}/Commands   # FIXME MacOS
		NO_DEFAULT_PATH )

	# find java, javac, jar and javadoc
	SET( JAVA_RUNTIME JAVA_RUNTIME-NOTFOUND )
	FIND_PROGRAM( JAVA_RUNTIME
		java
		${JAVA_BINSEARCH_PATH}
		NO_DEFAULT_PATH )

	SET( JAVA_COMPILE JAVA_COMPILE-NOTFOUND )
	FIND_PROGRAM( JAVA_COMPILE
		javac
		${JAVA_BINSEARCH_PATH}
		NO_DEFAULT_PATH )

	SET( JAVA_ARCHIVE JAVA_ARCHIVE-NOTFOUND )
	FIND_PROGRAM( JAVA_ARCHIVE
		jar
		${JAVA_BINSEARCH_PATH}
		NO_DEFAULT_PATH )

	SET( JAVA_DOC JAVA_DOC-NOTFOUND )
	FIND_PROGRAM( JAVA_DOC
		javadoc
		${JAVA_BINSEARCH_PATH}
		NO_DEFAULT_PATH )

	# abort if not found
	IF( NOT JAVA_RUNTIME AND NOT JAVA_FIND_QUIETLY )
		MESSAGE( STATUS "Could not find java!!" )
	ENDIF()
	IF( NOT JAVA_COMPILE AND NOT JAVA_FIND_QUIETLY )
		MESSAGE( STATUS "Could not find javac!!" )
	ENDIF()
	IF( NOT JAVA_ARCHIVE AND NOT JAVA_FIND_QUIETLY )
		MESSAGE( STATUS "Could not find jar!!" )
	ENDIF()
	IF( NOT JAVA_DOC AND NOT JAVA_FIND_QUIETLY )
		MESSAGE( STATUS "Could not find javadoc!!" )
	ENDIF()
ENDIF()

IF( JAVA_RUNTIME AND JAVA_COMPILE AND JAVA_ARCHIVE )

	SET( JAVA_FOUND TRUE )

	# parse the output of java -version
	EXEC_PROGRAM( "${JAVA_RUNTIME}" ARGS "-version"
			OUTPUT_VARIABLE out_tmp
			RETURN_VALUE out_ret )

	IF( out_ret )
		IF( NOT JAVA_FIND_QUIETLY )
			MESSAGE( STATUS "Error executing java -version!! Java version variables will not be set!!" )
		ENDIF()
	ELSE()
		# extract major/minor version and patch level from "java -version" output
		STRING( REGEX REPLACE ".* version \"([0-9]+)\\.[0-9]+\\.[0-9]+.*"
				"\\1" JAVA_MAJOR_VERSION "${out_tmp}" )
		STRING( REGEX REPLACE ".* version \"[0-9]+\\.([0-9]+)\\.[0-9]+.*"
				"\\1" JAVA_MINOR_VERSION "${out_tmp}" )
		STRING( REGEX REPLACE ".* version \"[0-9]+\\.[0-9]+\\.([0-9]+).*"
				"\\1" JAVA_PATCH_LEVEL "${out_tmp}" )

		SET( JAVA_VERSION "${JAVA_MAJOR_VERSION}.${JAVA_MINOR_VERSION}.${JAVA_PATCH_LEVEL}" )

		IF( NOT JAVA_FIND_QUIETLY )
			MESSAGE( STATUS "Java version ${JAVA_VERSION} configured successfully!" )
		ENDIF()
	ENDIF()
ELSE()
	IF( JAVA_FIND_REQUIRED )
		MESSAGE( FATAL_ERROR "Failed configuring Java!!" )
	ENDIF()
	IF( NOT JAVA_FIND_QUIETLY )
		MESSAGE( STATUS "Failed configuring Java!!" )
	ENDIF()
ENDIF()

