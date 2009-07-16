set(AIDIR ${DATADIR} CACHE STRING "Where AIs are installed")

add_definitions(-DBUILDING_AI)

set(rts "${CMAKE_SOURCE_DIR}/rts")

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

set(AI_FIND_QUIETLY FALSE)

# Assemble common additional native AI sources
list(APPEND ai_common ${rts}/Game/GameVersion)

# Assemble the Legacy C++ AI sources
aux_source_directory("Wrappers/LegacyCpp"          legacyCppAiSrcFiles)
aux_source_directory("${ai_common}"                legacyCppAiSrcFiles)
aux_source_directory("${rts}/System/float3"        legacyCppAiSrcFiles)
aux_source_directory("${rts}/Sim/Misc/DamageArray" legacyCppAiSrcFiles)

# Assemble the creg sources
aux_source_directory("${rts}/System/creg"          creg)

# Compile the Legacy C++ AI wrapper static library
add_library(legacycpp      STATIC ${legacyCppAiSrcFiles})

# Compile the Legacy C++ AI wrapper static library with creg support
add_library(legacycpp-creg STATIC ${legacyCppAiSrcFiles} ${creg})
set_target_properties(legacycpp-creg PROPERTIES COMPILE_FLAGS "-DUSING_CREG")

# Assemble the CUtils sources
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils CUtils_SRC)


# Find all CMakeLists.txt files in sub-directories
macro    (GetListOfSubModules list_var)
	file(GLOB ${list_var} RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" FOLLOW_SYMLINKS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

	# Strip away the "/CMakeLists.txt" parts, so we end up with just a list of dirs,
	# for example: AAI;RAI;KAIK
	string(REPLACE "//CMakeLists.txt" ";" ${list_var} ${${list_var}})
endmacro (GetListOfSubModules list_var)

# Gets the version from a text file.
# (actually just reads the text file content into a variable)
macro    (GetVersionFromFile vers_var file_var)
	if    (EXISTS ${file_var})
		file(STRINGS "${file_var}" ${vers_var} LIMIT_COUNT 1)
	else  (EXISTS ${file_var})
		set(${vers_var} "UNKNOWN_VERSION")
	endif (EXISTS ${file_var})
endmacro (GetVersionFromFile vers_var file_var)


Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)
