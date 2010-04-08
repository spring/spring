### AI
#
# Global variables set in this file:
# * AI_EXCLUDE_REGEX
# * AI_FIND_QUIETLY
#
# Functions and macros defined in this file:
# * SetGlobal
# * GetListOfSubModules
# * GetVersionFromFile
# * GetLastPathPart
#

add_definitions(-DBUILDING_AI)


### User-definable build options
set (AI_EXCLUDE_REGEX "XXXXXXXX" CACHE STRING "Which Skirmish AIs not to build (none by default, example: \"NTai|Null.*AI\", see also: AI_TYPES")
set (AI_FIND_QUIETLY  FALSE      CACHE BOOL   "Whether to find AI Interfaces and Skirmish AIs quietly")

set(rts             "${CMAKE_SOURCE_DIR}/rts")

include_directories(
	${rts}
	${rts}/System
	${rts}/ExternalAI/Interface
	Wrappers
	${SDL_INCLUDE_DIR})


# Set these for Interfaces and AIs with C sources
set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2}")
set(CMAKE_C_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3}")
set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_CXX_FLAGS_PROFILE}")

remove_definitions(-DSTREFLOP_SSE)
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)

if    (WIN32)
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--kill-at -Wl,--add-stdcall-alias")
endif (WIN32)


# Assemble common additional native AI sources
list(APPEND ai_common_SRC ${rts}/Game/GameVersion)

# Assemble the creg sources
aux_source_directory("${rts}/System/creg" creg_SRC)


################################################################################
### BEGINN: MACROS_AND_FUNCTIONS
# Define macros and functions to be used in this file and by Java Skirmish AIs

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

### END: MACROS_AND_FUNCTIONS
################################################################################


# Wrappers have to come first cause the Interfaces use the CUtils Wrapper too
Add_Subdirectory(Wrappers)
Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)
