################################################################################
# cmake module for finding JAVA
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
# JAVA_MAJOR_VERSION
# JAVA_MINOR_VERSION
# JAVA_PATCH_LEVEL
#
# JAVA_VERSION - ${JAVA_MAJOR_VERSION}.${JAVA_MINOR_VERSION}.${JAVA_PATCH_LEVEL}
#
# @author Jan Engels, DESY
################################################################################

SET( JAVA_FOUND FALSE )
MARK_AS_ADVANCED( JAVA_FOUND )

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
        IF( UNIX AND NOT APPLE )
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
                            STRING( REGEX REPLACE ".* symbolic link to[^/]+([^'`´]+).*" "\\1" out_regex "${out_tmp}" )
                            IF( NOT JAVA_FIND_QUIETLY )
                                MESSAGE( STATUS "Java binary ${java_bin} is a symbolic link to ${out_regex}" )
                            ENDIF()
                            SET( java_bin "${out_regex}" )
                        ENDIF()
                    ENDIF()
                ENDWHILE()

                # check if something went wrong
                IF( NOT java_link_error )
                    # if not set JAVA_RUNTIME to the dereferenced link
                    SET( JAVA_RUNTIME "${java_bin}" )
                ENDIF()
            ENDIF()
        ENDIF()
        
        # extract java home path out of full path to java runtime
        STRING( REGEX REPLACE "(.*)\\/[^/]+\\/java$" "\\1" JAVA_HOME ${JAVA_RUNTIME} )
        # extract java bin path out of full path to java runtime
        STRING( REGEX REPLACE "(.*)\\/java$" "\\1" JAVA_BIN_PATH ${JAVA_RUNTIME} )
    ELSE()
        IF( NOT JAVA_FIND_QUIETLY )
            MESSAGE( STATUS "Failed to autodetect Java!!" )
        ENDIF()
    ENDIF()
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

    SET( JAVA_BIN_PATH JAVA_BIN_PATH-NOTFOUND )
    FIND_PATH( JAVA_BIN_PATH
        java
        ${JAVA_HOME}/bin
        ${JAVA_HOME}/Commands   # FIXME MacOS
        NO_DEFAULT_PATH )
    
    IF( NOT JAVA_BIN_PATH AND NOT JAVA_FIND_QUIETLY )
        MESSAGE( STATUS "${JAVA_HOME} is not a valid path for Java!!" )
    ENDIF()

    IF( JAVA_BIN_PATH )
        # find java, javac, jar and javadoc
        SET( JAVA_RUNTIME JAVA_RUNTIME-NOTFOUND )
        FIND_PROGRAM( JAVA_RUNTIME
            java
            ${JAVA_BIN_PATH}
            NO_DEFAULT_PATH )
      
        SET( JAVA_COMPILE JAVA_COMPILE-NOTFOUND )
        FIND_PROGRAM( JAVA_COMPILE
            javac
            ${JAVA_BIN_PATH}
            NO_DEFAULT_PATH )
        
        SET( JAVA_ARCHIVE JAVA_ARCHIVE-NOTFOUND )
        FIND_PROGRAM( JAVA_ARCHIVE
            jar
            ${JAVA_BIN_PATH}
            NO_DEFAULT_PATH )
         
        SET( JAVA_DOC JAVA_DOC-NOTFOUND )
        FIND_PROGRAM( JAVA_DOC
            javadoc
            ${JAVA_BIN_PATH}
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

        # display info
        MESSAGE( STATUS "Java version ${JAVA_VERSION} configured successfully!" )
    ENDIF()
ELSE()
    IF( JAVA_FIND_REQUIRED )
        MESSAGE( FATAL_ERROR "Failed configuring Java!!" )
    ENDIF()
    IF( NOT JAVA_FIND_QUIETLY )
        MESSAGE( STATUS "Failed configuring Java!!" )
    ENDIF()
ENDIF()

