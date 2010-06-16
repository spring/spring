# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# Spring CMake utilities
# ----------------------
#
# Variables set in this file:
# * PATH_SEP_H
# * PATH_SEP_T
# * PATH_DELIM_H
# * PATH_DELIM_T
# * ABS_DIR_REGEX_H
# * ABS_DIR_REGEX_T
# * PIC_FLAG
#
# Functions and macros defined in this file:
# * FixLibName
# * SetGlobal
# * GetListOfSubModules
# * GetVersionFromFile
# * GetLastPathPart
#


if   (CMAKE_HOST_WIN32)
	set(PATH_SEP_H      "\\")
	set(PATH_DELIM_H    ";")
	set(ABS_DIR_REGEX_H "^[a-zA-Z]:\\")
else (CMAKE_HOST_WIN32)
	set(PATH_SEP_H      "/")
	set(PATH_DELIM_H    ":")
	set(ABS_DIR_REGEX_H "^/")
endif (CMAKE_HOST_WIN32)
if    (WIN32)
	set(PATH_SEP_T      "\\")
	set(PATH_DELIM_T    ";")
	set(ABS_DIR_REGEX_T "^[a-zA-Z]:\\")
else  (WIN32)
	set(PATH_SEP_T      "/")
	set(PATH_DELIM_T    ":")
	set(ABS_DIR_REGEX_T "^/")
endif (WIN32)


# define the fPic compiler flag
if     (APPLE)
	set (PIC_FLAG "-fPIC")
elseif (MINGW)
	set (PIC_FLAG "")
else   ()
	set (PIC_FLAG "-fpic")
endif  ()


# This is needed because CMake, or at least some versions of it (eg. 2.8),
# falsely use the ".so" suffix unde Mac OS X for MODULE's
macro    (FixLibName targetName)
	IF    (UNIX)
		SET_TARGET_PROPERTIES(${targetName} PROPERTIES PREFIX "lib")
		IF    (APPLE)
			SET_TARGET_PROPERTIES(${targetName} PROPERTIES SUFFIX ".dylib")
		ENDIF (APPLE)
	ENDIF (UNIX)
endmacro (FixLibName targetName)


# Create an install target which installs multiple sub-projects.
# Sub-projects have to be specified by paths relative to CMAKE_SOURCE_DIR.
# All install instructions in the specified dirs (recursively) are executed.
# example:
# 	set(myInstallDirs
# 			"rts/builds/default"
# 			"tools/unitsync"
# 			"cont"
# 			"AI"
# 		)
# 	set(myInstallDeps
# 			spring
# 			gamedata
# 			unitsync
# 			C-AIInterface
# 			NullAI
# 			KAIK
# 			ArchiveMover
# 		)
# 	CreateInstallTarget(myPkg myInstallDeps myInstallDirs)
# This creates a new target "install-myPkg"
macro    (CreateInstallTarget targetName var_list_depends var_list_instDirs)
	# Assemble the list of commands
	set(installCmds "")
	foreach    (instDir ${${var_list_instDirs}})
		if    (NOT EXISTS "${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt")
			message(FATAL_ERROR "Not a valid dir for installer target: ${instDir}, \"${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt\" does not exist.")
		endif (NOT EXISTS "${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt")
		set(installCmds ${installCmds} 
			COMMAND "${CMAKE_COMMAND}"
				"-P" "${CMAKE_BINARY_DIR}/${instDir}/cmake_install.cmake"
				# NOTE: The following does not work in CMake 2.6.4
				#"-DCMAKE_INSTALL_COMPONENT=${targetName}"
			)
	endforeach (instDir)

	# Make sure we do have commands at all
	if    ("${installCmds}" STREQUAL "")
		message(FATAL_ERROR "No valid install dirs supplied.")
	endif ("${installCmds}" STREQUAL "")

	# Create a custom install target
	add_custom_target(install-${targetName}
		${installCmds}
		WORKING_DIRECTORY
			"${CMAKE_BINARY_DIR}"
		COMMENT
			"  ${targetName}: Installing ..." VERBATIM
		)
	# This also works for custom targets
	add_dependencies(install-${targetName} ${${var_list_depends}})
endmacro (CreateInstallTarget targetName)


# Sets a variable in global scope
function    (SetGlobal var value)
	set(${var} "${value}" CACHE INTERNAL "" FORCE)
endfunction (SetGlobal)


# Makes variables available in global scope
# MakeGlobal(var0 [var1 [var2 [var3 ...]]])
function    (MakeGlobal)
	foreach    (var ${ARGV})
		SetGlobal(${var} "${${var}}")
	endforeach (var)
endfunction (MakeGlobal)


# Find all CMakeLists.txt files in sub-directories
macro    (GetListOfSubModules list_var)
	file(GLOB ${list_var} RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" FOLLOW_SYMLINKS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

	# Strip away the "/CMakeLists.txt" parts, so we end up with just a list of dirs,
	# for example: AAI;RAI;KAIK
	string(REPLACE "//CMakeLists.txt" "" ${list_var} "${${list_var}}")
endmacro (GetListOfSubModules list_var)


# Gets the version from a text file.
# (actually just reads the text file content into a variable)
macro    (GetVersionFromFile vers_var vers_file)
	if    (EXISTS ${vers_file})
		file(STRINGS "${vers_file}" ${vers_var} LIMIT_COUNT 1)
	else  (EXISTS ${vers_file})
		set(${vers_var} "UNKNOWN_VERSION")
	endif (EXISTS ${vers_file})
endmacro (GetVersionFromFile vers_var vers_file)


# Returns the name of the dir or file specified by a path.
# example: "/A/B/C" -> "C"
macro    (GetLastPathPart part_var dir)
	string(REGEX REPLACE ".*[\\/]" "" ${part_var} ${dir})
endmacro (GetLastPathPart part_var dir)


# Create an absolute directory from a base- and a relative-dir
function    (MakeAbsolute absDir_var baseDir relDir)
	set(_absDir "${baseDir}")
	if    (NOT "${relDir}" STREQUAL "")
		set(_absDir "${_absDir}/${relDir}")
	endif (NOT "${relDir}" STREQUAL "")
	set(${absDir_var} ${_absDir} PARENT_SCOPE)
endfunction (MakeAbsolute)


# Recursively lists all native source files in a given directory,
# relative to _relDir, or absolut, if _relDir == "".
macro    (GetNativeSourcesRecursive _var _dir _relDir)
	set(NATIVE_SOURCE_EXTENSIONS ".c;.cpp;.c++;.cxx")
	foreach    (_ext ${NATIVE_SOURCE_EXTENSIONS})
		# Recursively get sources for source extension _ext
		if    ("${_relDir}" STREQUAL "")
			file(GLOB_RECURSE _sources FOLLOW_SYMLINKS "${_dir}/*${_ext}")
		else  ("${_relDir}" STREQUAL "")
			file(GLOB_RECURSE _sources RELATIVE "${_relDir}" FOLLOW_SYMLINKS "${_dir}/*${_ext}")
		endif ("${_relDir}" STREQUAL "")
		# Concatenate to previous results
		if    ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
			set(${_var} "${${_var}}${_sources}")
		else  ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
			set(${_var} "${${_var}};${_sources}")
		endif ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
	endforeach (_ext)
endmacro (GetNativeSourcesRecursive _var _dir _relDir)

