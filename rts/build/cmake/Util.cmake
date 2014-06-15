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
# * CreateInstallTarget
# * RemoveString
# * RemoveFlag
# * SetGlobal
# * MakeGlobal
# * GetListOfSubModules
# * GetVersionFromFile
# * GetLastPathPart
# * MakeAbsolute
# * GetVersionPlusDepFile
# * GetNativeSourcesRecursive
# * CheckMinCMakeVersion
#


If   (CMAKE_HOST_WIN32)
	Set(PATH_SEP_H      "\\")
	Set(PATH_DELIM_H    ";")
	Set(ABS_DIR_REGEX_H "^[a-zA-Z]:\\")
Else (CMAKE_HOST_WIN32)
	Set(PATH_SEP_H      "/")
	Set(PATH_DELIM_H    ":")
	Set(ABS_DIR_REGEX_H "^/")
EndIf (CMAKE_HOST_WIN32)
If    (WIN32)
	Set(PATH_SEP_T      "\\")
	Set(PATH_DELIM_T    ";")
	Set(ABS_DIR_REGEX_T "^[a-zA-Z]:\\")
Else  (WIN32)
	Set(PATH_SEP_T      "/")
	Set(PATH_DELIM_T    ":")
	Set(ABS_DIR_REGEX_T "^/")
EndIf (WIN32)


# define the fPic compiler flag
If     (APPLE)
	Set(PIC_FLAG "-fPIC")
ElseIf (MINGW)
	Set(PIC_FLAG "")
Else   ()
	if (CMAKE_SIZEOF_VOID_P EQUAL 8) # add fpic flag on 64 bit platforms
		Set(PIC_FLAG "-fpic")
	else () #no fpic needed on 32bit
		set(CMAKE_POSITION_INDEPENDENT_CODE FALSE)
		Set(PIC_FLAG "")
	endif()
EndIf  ()


# This is needed because CMake, or at least some versions of it (eg. 2.8),
# falsely use the ".so" suffix under Mac OS X for MODULE's
Macro    (FixLibName targetName)
	If    (UNIX)
		Set_TARGET_PROPERTIES(${targetName} PROPERTIES PREFIX "lib")
		If    (APPLE)
			Set_TARGET_PROPERTIES(${targetName} PROPERTIES SUFFIX ".dylib")
		EndIf (APPLE)
	EndIf (UNIX)
EndMacro (FixLibName targetName)


# Create an install target which installs multiple sub-projects.
# Sub-projects have to be specified by paths relative to CMAKE_SOURCE_DIR.
# All install instructions in the specified dirs (recursively) are executed.
# example:
# 	Set(myInstallDirs
# 			"rts/builds/default"
# 			"tools/unitsync"
# 			"cont"
# 			"AI"
# 		)
# 	Set(myInstallDeps
# 			spring
# 			gamedata
# 			unitsync
# 			C-AIInterface
# 			NullAI
# 			KAIK
# 		)
# 	CreateInstallTarget(myPkg myInstallDeps myInstallDirs)
# This creates a new target "install-myPkg"
Macro    (CreateInstallTarget targetName var_list_depends var_list_instDirs)
	# Assemble the list of commands
	Set(installCmds "")
	ForEach    (instDir ${${var_list_instDirs}})
		If    (NOT EXISTS "${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt")
			Message(FATAL_ERROR "Not a valid dir for installer target: ${instDir}, \"${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt\" does not exist.")
		EndIf (NOT EXISTS "${CMAKE_SOURCE_DIR}/${instDir}/CMakeLists.txt")
		Set(installCmds ${installCmds} 
			COMMAND "${CMAKE_COMMAND}"
				"-P" "${CMAKE_BINARY_DIR}/${instDir}/cmake_install.cmake"
				# NOTE: The following does not work in CMake 2.6.4
				#"-DCMAKE_INSTALL_COMPONENT=${targetName}"
			)
	EndForEach (instDir)

	# Make sure we do have commands at all
	If    ("${installCmds}" STREQUAL "")
		Message(FATAL_ERROR "No valid install dirs supplied.")
	EndIf ("${installCmds}" STREQUAL "")

	# Create a custom install target
	Add_Custom_Target(install-${targetName}
		${installCmds}
		WORKING_DIRECTORY
			"${CMAKE_BINARY_DIR}"
		COMMENT
			"  ${targetName}: Installing ..." VERBATIM
		)
	# This also works for custom targets
	Add_Dependencies(install-${targetName} ${${var_list_depends}})
EndMacro (CreateInstallTarget targetName)


# Removes a given string from a variable
Macro    (RemoveString var str)
	String(REPLACE "${str}" "" ${var} "${${var}}")
EndMacro (RemoveString)


# Removes a compiler flag from all commonly used vars used for that purpose
Macro    (RemoveFlag flag)
	RemoveString(CMAKE_CXX_FLAGS "${flag}")
	RemoveString(CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} "${flag}")
	RemoveString(CMAKE_C_FLAGS "${flag}")
	RemoveString(CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} "${flag}")
	RemoveString(CMAKE_EXE_LINKER_FLAGS "${flag}")
	RemoveString(CMAKE_MODULE_LINKER_FLAGS "${flag}")
EndMacro (RemoveFlag)


# Sets a variable in global scope
Function    (SetGlobal var value)
	Set(${var} "${value}" CACHE INTERNAL "" FORCE)
	Mark_As_Advanced(${var})
EndFunction (SetGlobal)


# Makes variables available in global scope
# MakeGlobal(var0 [var1 [var2 [var3 ...]]])
Function    (MakeGlobal)
	ForEach    (var ${ARGV})
		SetGlobal(${var} "${${var}}")
	EndForEach (var)
EndFunction (MakeGlobal)


# Find all CMakeLists.txt files in sub-directories
Macro    (GetListOfSubModules list_var)
	File(GLOB ${list_var} RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" FOLLOW_SYMLINKS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

	# Strip away the "/CMakeLists.txt" parts, so we end up with just a list of dirs,
	# for example: AAI;RAI;KAIK
	String(REPLACE "//CMakeLists.txt" "" ${list_var} "${${list_var}}")
EndMacro (GetListOfSubModules list_var)


# Gets the version from a text file.
# (actually just reads the text file content into a variable)
Macro    (GetVersionFromFile vers_var vers_file)
	If    (EXISTS ${vers_file})
		File(STRINGS "${vers_file}" ${vers_var} LIMIT_COUNT 1)
	Else  (EXISTS ${vers_file})
		Set(${vers_var} "UNKNOWN_VERSION")
	EndIf (EXISTS ${vers_file})
EndMacro (GetVersionFromFile vers_var vers_file)


# Returns the name of the dir or file specified by a path.
# example: "/A/B/C" -> "C"
Macro    (GetLastPathPart part_var dir)
	String(REGEX REPLACE ".*[\\/]" "" ${part_var} ${dir})
EndMacro (GetLastPathPart part_var dir)


# Create an absolute directory from a base- and a relative-dir
Function    (MakeAbsolute absDir_var baseDir relDir)
	Set(_absDir "${baseDir}")
	If    (NOT "${relDir}" STREQUAL "")
		Set(_absDir "${_absDir}/${relDir}")
	EndIf (NOT "${relDir}" STREQUAL "")
	Set(${absDir_var} ${_absDir} PARENT_SCOPE)
EndFunction (MakeAbsolute)


# Gets the version from a text file (${CMAKE_CURRENT_SOURCE_DIR}/VERSION),
# and prepare a file for dependency tracking.
# The project will reconfigure whenever the VERSION file gets touched.
Macro    (GetVersionPlusDepFile vers_var versDepFile_var)
	Set(myVersionFile    "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
	Set(myVersionDepFile "${CMAKE_CURRENT_BINARY_DIR}/VERSION")
	If    (EXISTS ${myVersionFile})
		GetVersionFromFile(${vers_var} "${myVersionFile}")
		Configure_File("${myVersionFile}" "${myVersionDepFile}" COPYONLY)
		Set_source_files_properties("${myVersionDepFile}" PROPERTIES HEADER_FILE_ONLY TRUE)
		Set_source_files_properties("${myVersionDepFile}" PROPERTIES GENERATED TRUE)
		Set(${versDepFile_var} "${myVersionDepFile}")
	Else  (EXISTS ${myVersionFile})
		Set(${vers_var}        "UNKNOWN_VERSION")
		Set(${versDepFile_var} "${myVersionFile}")
	EndIf (EXISTS ${myVersionFile})
EndMacro (GetVersionPlusDepFile vers_var versDepFile_var)


# Recursively lists all native source files in a given directory,
# relative to _relDir, or absolut, If _relDir == "".
Macro    (GetNativeSourcesRecursive _var _dir _relDir)
	Set(NATIVE_SOURCE_EXTENSIONS ".c;.cpp;.c++;.cxx")
	ForEach    (_ext ${NATIVE_SOURCE_EXTENSIONS})
		# Recursively get sources for source extension _ext
		If    ("${_relDir}" STREQUAL "")
			File(GLOB_RECURSE _sources FOLLOW_SYMLINKS "${_dir}/*${_ext}")
		Else  ("${_relDir}" STREQUAL "")
			File(GLOB_RECURSE _sources RELATIVE "${_relDir}" FOLLOW_SYMLINKS "${_dir}/*${_ext}")
		EndIf ("${_relDir}" STREQUAL "")
		# Concatenate to previous results
		If    ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
			Set(${_var} "${${_var}}${_sources}")
		Else  ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
			Set(${_var} "${${_var}};${_sources}")
		EndIf ("${_sources}" STREQUAL "" OR "${${_var}}" STREQUAL "")
	EndForEach (_ext)
EndMacro (GetNativeSourcesRecursive _var _dir _relDir)


# Check If the CMake version used is >= "major.minor.patch".
Macro    (CheckMinCMakeVersion res_var major minor patch)
	Set(${res_var} FALSE)
	If     (${CMAKE_MAJOR_VERSION} GREATER ${major})
		Set(${res_var} TRUE)
	ElseIf (${CMAKE_MAJOR_VERSION} EQUAL ${major})
		If     (${CMAKE_MINOR_VERSION} GREATER ${minor})
			Set(${res_var} TRUE)
		ElseIf (${CMAKE_MINOR_VERSION} EQUAL ${minor})
			If     (${CMAKE_PATCH_VERSION} GREATER ${patch})
				Set(${res_var} TRUE)
			ElseIf (${CMAKE_PATCH_VERSION} EQUAL ${patch})
				Set(${res_var} TRUE)
			EndIf  ()
		EndIf  ()
	EndIf  ()
EndMacro (CheckMinCMakeVersion res_var major minor patch)


# Tries to capture a specific regex group from a string.
# The regex has to match the whole string.
# @param pattern the regex to match to
# @param group starts at 1
# @param var to write the result to, "" in case of no match
# @param str the string that will be tried to be match
Macro    (CatchRegexGroup pattern group var str)
	String(REGEX MATCH "^${pattern}\$" "${var}_MATCH_TEST" "${str}")
	Set(${var} "")
	If     ("${${var}_MATCH_TEST}" STREQUAL "${str}")
		String(REGEX REPLACE "${pattern}" "\\${group}" ${var} "${str}")
	EndIf  ()
EndMacro (CatchRegexGroup)


# macro that adds "freetype-6 freetype6" to find_library on win32
Macro(FindFreetypeHack)
	if(WIN32)

PREFER_STATIC_LIBS()
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
UNPREFER_STATIC_LIBS()
	endif()
EndMacro()

