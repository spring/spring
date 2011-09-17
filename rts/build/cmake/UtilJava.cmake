# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# Spring Java related CMake utilities
# -----------------------------------
#
# Variables set in this file:
# * JAVA_GLOBAL_LIBS_DIRS
#
# Functions and macros defined in this file:
# * FindJavaLib
# * GetFirstSubDirName
# * CreateClasspath
# * ConcatClasspaths
# * IsMavenProject
# * FindManifestFile
#


if     (CMAKE_HOST_WIN32)
	set(JAVA_GLOBAL_LIBS_DIRS "${MINGWLIBS}")
else   (CMAKE_HOST_WIN32)
	set(JAVA_GLOBAL_LIBS_DIRS "/usr/share/java" "/usr/local/share/java")
endif  (CMAKE_HOST_WIN32)
MakeGlobal(JAVA_GLOBAL_LIBS_DIRS)


# Looks for a Java library (Jar file) in system wide search paths and in
# additional dirs supplied as argument.
macro    (FindJavaLib path_var libName additionalSearchDirs)
	find_file(${path_var} "${libName}.jar"
		PATHS ${additionalSearchDirs} ${JAVA_GLOBAL_LIBS_DIRS}
		DOC "Path to the Java library ${libName}.jar"
		NO_DEFAULT_PATH
		NO_CMAKE_FIND_ROOT_PATH
		)
	if    (NOT ${path_var})
		message(SEND_ERROR "${libName}.jar not found!")
	else  (NOT ${path_var})
		AIMessage(STATUS "Found ${libName}.jar: ${${path_var}}")
	endif (NOT ${path_var})
endmacro (FindJavaLib path_var libName additionalSearchDirs)


# Returns the name of the first sub-dir (in alphabetical descending order)
# under dir.
macro    (GetFirstSubDirName name_var dir)
	file(GLOB dirContent RELATIVE "${dir}" FOLLOW_SYMLINKS "${dir}/*")
	foreach    (dirPart ${dirContent})
		if    (IS_DIRECTORY "${dir}/${dirPart}")
			set(${name_var} ${dirPart})
			break()
		endif (IS_DIRECTORY "${dir}/${dirPart}")
	endforeach (dirPart)
endmacro (GetFirstSubDirName name_var dir)


# Recursively lists all JAR files in a given directory
# and concatenates them in a Java Classpath compatible way into a single string.
macro    (CreateClasspath classPath_var dir)
	file(GLOB_RECURSE ${classPath_var} FOLLOW_SYMLINKS "${dir}/*.jar")
	# Make sure we use the correct path delimitter for the compiling system
	string(REPLACE ";" "${PATH_DELIM_H}" ${classPath_var} "${${classPath_var}}")
endmacro (CreateClasspath classPath_var dir)


# Concatenates an arbritrary number of Java ClassPaths (may be empty).
function    (ConcatClasspaths resultingCP_var)
	set(${resultingCP_var} "")
	foreach    (cpPart ${ARGN})
		set(${resultingCP_var} "${${resultingCP_var}}${cpPart}${PATH_DELIM_H}")
	endforeach (cpPart)
	string(REGEX REPLACE "${PATH_DELIM_H}\$" "" ${resultingCP_var} "${${resultingCP_var}}")
	set(${resultingCP_var} "${${resultingCP_var}}" PARENT_SCOPE)
endfunction (ConcatClasspaths)


# Checks if a given directory is the root of a Maven project.
# The result variable will be set to TRUE or FALSE
function    (IsMavenProject dirToCheck result_var)
	set(${result_var} FALSE)
	if    (EXISTS "${dirToCheck}/pom.xml")
		set(${result_var} TRUE)
	endif (EXISTS "${dirToCheck}/pom.xml")
	set(${result_var} "${${result_var}}" PARENT_SCOPE)
endfunction (IsMavenProject)


# Look for a manifest.mf file in a few specific sub-dirs.
# This could be done with a simple find_file call,
# but that strangely does not find the file under win32,
# so we use this workaround
function    (FindManifestFile srcDir result_var)
	set(manifestSubdirs
		"/src/main/resources/META-INF/"
		"/src/"
		"/")
	set(${result_var} "${result_var}-NOTFOUND")
	if     (CMAKE_HOST_WIN32)
		foreach(subDir_var ${manifestSubdirs})
			if     (EXISTS "${srcDir}${subDir_var}manifest.mf")
				set(${result_var} "${srcDir}${subDir_var}manifest.mf")
				break()
			endif  (EXISTS "${srcDir}${subDir_var}manifest.mf")
		endforeach(subDir_var)
	else   (CMAKE_HOST_WIN32)
		find_file(${result_var}
			NAMES "manifest.mf" "MANIFEST.MF"
			PATHS "${srcDir}"
			PATH_SUFFIXES ${manifestSubdirs}
			NO_DEFAULT_PATH)
	endif  (CMAKE_HOST_WIN32)
	set(${result_var} ${${result_var}} PARENT_SCOPE)
endfunction (FindManifestFile)

