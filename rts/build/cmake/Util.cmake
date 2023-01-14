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
# * fix_lib_name
# * create_install_target
# * remove_string
# * remove_flag
# * set_global
# * make_global
# * get_list_of_submodules
# * get_version_from_file
# * get_last_path_part
# * make_absolute
# * get_version_plus_dep_file
# * get_native_sources_recursive
# * catch_regex_group
# * find_freetype_hack
# * make_global_var


if (CMAKE_HOST_WIN32)
	set(PATH_SEP_H      "\\")
	set(PATH_DELIM_H    ";")
	set(ABS_DIR_REGEX_H "^[a-zA-Z]:\\")
else ()
	set(PATH_SEP_H      "/")
	set(PATH_DELIM_H    ":")
	set(ABS_DIR_REGEX_H "^/")
endif ()
if (WIN32)
	set(PATH_SEP_T      "\\")
	set(PATH_DELIM_T    ";")
	set(ABS_DIR_REGEX_T "^[a-zA-Z]:\\")
else ()
	set(PATH_SEP_T      "/")
	set(PATH_DELIM_T    ":")
	set(ABS_DIR_REGEX_T "^/")
endif ()


# define the fPic compiler flag
if (APPLE)
	set(PIC_FLAG "-fPIC")
elseif (MINGW)
	set(PIC_FLAG "")
else ()
	if (CMAKE_SIZEOF_VOID_P EQUAL 8) # add fpic flag on 64 bit platforms
		set(PIC_FLAG "-fpic")
	else ()
		set(CMAKE_POSITION_INDEPENDENT_CODE FALSE)
		set(PIC_FLAG "")
	endif ()
endif ()


# This is needed because CMake, or at least some versions of it (eg. 2.8),
# falsely use the ".so" suffix under Mac OS X for MODULE's
macro (fix_lib_name targetName)
	if (UNIX)
		set_target_properties(${targetName} PROPERTIES PREFIX "lib")
		if (APPLE)
			set_target_properties(${targetName} PROPERTIES SUFFIX ".dylib")
		endif ()
	endif ()
endmacro ()


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
# 		)
# 	create_install_target(myPkg myInstallDeps myInstallDirs)
# This creates a new target "install-myPkg"
macro (create_install_target targetName var_list_depends var_list_instDirs)
	# Assemble the list of commands
	set(installCmds "")
	foreach    (instDir ${${var_list_instDirs}})
		if (NOT EXISTS "${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt")
			message (FATAL_ERROR "Not a valid dir for installer target: ${instDir}, \"${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt\" does not exist.")
		endif ()
		set(installCmds ${installCmds}
			COMMAND "${CMAKE_COMMAND}"
				"-P" "${CMAKE_BINARY_DIR}/${instDir}/cmake_install.cmake"
				# NOTE: The following does not work in CMake 2.6.4
				#"-DCMAKE_INSTALL_COMPONENT=${targetName}"
			)
	endforeach ()

	# Make sure we do have commands at all
	if ("${installCmds}" STREQUAL "")
		message (FATAL_ERROR "No valid install dirs supplied.")
	endif ()

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
endmacro ()


# Removes a given string from a variable
macro (remove_string var str)
	string(REPLACE "${str}" "" ${var} "${${var}}")
endmacro ()


# Removes a compiler flag from all commonly used vars used for that purpose
macro (remove_flag flag)
	remove_string(CMAKE_CXX_FLAGS "${flag}")
	remove_string(CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} "${flag}")
	remove_string(CMAKE_C_FLAGS "${flag}")
	remove_string(CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} "${flag}")
	remove_string(CMAKE_EXE_LINKER_FLAGS "${flag}")
	remove_string(CMAKE_MODULE_LINKER_FLAGS "${flag}")
endmacro ()


# Sets a variable in global scope
function    (set_global var value)
	set(${var} "${value}" CACHE INTERNAL "" FORCE)
	mark_as_advanced(${var})
endfunction ()


# Makes variables available in global scope
# make_global(var0 [var1 [var2 [var3 ...]]])
function    (make_global)
	foreach    (var ${ARGV})
		set_global(${var} "${${var}}")
	endforeach ()
endfunction ()


# Find all CMakeLists.txt files in sub-directories
macro (get_list_of_submodules list_var)
	file(GLOB ${list_var} RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
	# Strip away the "/CMakeLists.txt" parts, so we end up with just a list of dirs,
	# for example: AAI;RAI;KAIK
	# GLOB can prefix with "//" or "/" (perhaps changed in cmake 3.1.0), this double replace will support both "//" and "/"
	string(REPLACE "/CMakeLists.txt" "" ${list_var} "${${list_var}}")
	string(REPLACE "/" "" ${list_var} "${${list_var}}")
endmacro ()


# Gets the version from a text file.
# (actually just reads the text file content into a variable)
macro (get_version_from_file vers_var vers_file)
	if (EXISTS ${vers_file})
		file(STRINGS "${vers_file}" ${vers_var} LIMIT_COUNT 1)
	else ()
		set(${vers_var} "UNKNOWN_VERSION")
	endif ()
endmacro ()


# Returns the name of the dir or file specified by a path.
# example: "/A/B/C" -> "C"
macro (get_last_path_part part_var dir)
	string(REGEX REPLACE ".*[\\/]" "" ${part_var} ${dir})
endmacro ()


# Create an absolute directory from a base- and a relative-dir
function    (make_absolute absDir_var baseDir relDir)
	set(_absDir "${baseDir}")
	if (NOT "${relDir}" STREQUAL "")
		set(_absDir "${_absDir}/${relDir}")
	endif ()
	set(${absDir_var} ${_absDir} PARENT_SCOPE)
endfunction ()


# Gets the version from a text file (${CMAKE_CURRENT_SOURCE_DIR}/VERSION),
# and prepare a file for dependency tracking.
# The project will reconfigure whenever the VERSION file gets touched.
macro (get_version_plus_dep_file vers_var versDepFile_var)
	set(myVersionFile    "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
	set(myVersionDepFile "${CMAKE_CURRENT_BINARY_DIR}/VERSION")
	if (EXISTS ${myVersionFile})
		get_version_from_file(${vers_var} "${myVersionFile}")
		configure_file("${myVersionFile}" "${myVersionDepFile}" COPYONLY)
		set_source_files_properties("${myVersionDepFile}" PROPERTIES HEADER_FILE_ONLY TRUE)
		set_source_files_properties("${myVersionDepFile}" PROPERTIES GENERATED TRUE)
		set(${versDepFile_var} "${myVersionDepFile}")
	else ()
		set(${vers_var}        "UNKNOWN_VERSION")
		set(${versDepFile_var} "${myVersionFile}")
	endif ()
endmacro ()


# Recursively lists all native source files in a given directory,
# relative to _relDir, or absolut, If _relDir == "".
macro (get_native_sources_recursive _var _dir _relDir)
	set(NATIVE_SOURCE_EXTENSIONS ".c;.cpp;.c++;.cxx")
	foreach    (_ext ${NATIVE_SOURCE_EXTENSIONS})
		# Recursively get sources for source extension _ext
		if ("${_relDir}" STREQUAL "")
			file(GLOB_RECURSE _sources FOLLOW_SYMLINKS "${_dir}/*${_ext}")
		else ()
			file(GLOB_RECURSE _sources RELATIVE "${_relDir}" FOLLOW_SYMLINKS "${_dir}/*${_ext}")
		endif ()
		# Concatenate to previous results
		if ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
			set(${_var} "${${_var}}${_sources}")
		else ()
			set(${_var} "${${_var}};${_sources}")
		endif ()
	endforeach ()
endmacro ()

# Tries to capture a specific regex group from a string.
# The regex has to match the whole string.
# @param pattern the regex to match to
# @param group starts at 1
# @param var to write the result to, "" in case of no match
# @param str the string that will be tried to be match
macro (catch_regex_group pattern group var str)
	string(REGEX MATCH "^${pattern}\$" "${var}_MATCH_TEST" "${str}")
	set(${var} "")
	if ("${${var}_MATCH_TEST}" STREQUAL "${str}")
		string(REGEX REPLACE "${pattern}" "\\${group}" ${var} "${str}")
	endif ()
endmacro ()


# macro that adds "freetype-6 freetype6" to find_library on win32
macro (find_freetype_hack)
	if (WIN32)
		prefer_static_libs()
		find_library(FREETYPE_LIBRARY
		  NAMES freetype libfreetype freetype219 freetype-6 freetype6
		  HINTS
		    ENV FREETYPE_DIR
		  PATH_SUFFIXES lib
		  PATHS
		  /usr/X11R6
		  /usr/local/X11R6
		  /usr/local/X11
		  /usr/freeware
		)
		unprefer_static_libs()
	endif ()
endmacro ()


# make a var global (not cached in CMakeCache.txt!)
# both calls are required, else the variable is empty
# http://www.cmake.org/Bug/view.php?id=15093
macro (make_global_var varname)
        set(${varname} ${ARGN} PARENT_SCOPE)
        set(${varname} ${ARGN})
endmacro ()


macro(make_target_invisible targetname)
	set_target_properties(${targetname} PROPERTIES CXX_VISIBILITY_PRESET hidden)
	set_target_properties(${targetname} PROPERTIES C_VISIBILITY_PRESET hidden)
	set_target_properties(${targetname} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
endmacro()
