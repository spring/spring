# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find windres executable
# manipulate Windows resources
#
#  WINDRES_BIN    - will be set to the windres executable (eg. windres.exe)
#  WINDRES_FOUND  - TRUE if windres was found

if (MINGW)
	# Already in cache, be silent
	if (WINDRES_BIN)
		SET(Windres_FIND_QUIETLY TRUE)
	endif ()

	FIND_PROGRAM(WINDRES_BIN
		NAMES
			windres
			x86_64-w64-mingw32.static-windres
			i686-w64-mingw32.static.posix-windres
			i686-w64-mingw32-windres
			i586-mingw32msvc-windres
			i586-pc-mingw32-windres
			i686-pc-mingw32-windres
			i686-mingw32-windres
			${CMAKE_RC_COMPILER}
		DOC "path to mingw's windres executable"
		)

	INCLUDE(FindPackageHandleStandardArgs)

	# handle the QUIETLY and REQUIRED arguments and set WINDRES_FOUND to TRUE if
	# all listed variables are TRUE
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(Windres DEFAULT_MSG WINDRES_BIN)

	MARK_AS_ADVANCED(WINDRES_BIN)


	macro (CreateResourceCompileCommand out_var dirIn fileIn fileOut)
		if (WINDRES_FOUND)
			ADD_CUSTOM_COMMAND(
				OUTPUT
					"${fileOut}"
				DEPENDS
					"${fileIn}"
				COMMAND
					"${WINDRES_BIN}"
						"-I${dirIn}"
						"-i${fileIn}"
						"-o" "${fileOut}"
						"-v"
				)
			SET_SOURCE_FILES_PROPERTIES(${fileOut} PROPERTIES
				GENERATED      TRUE
				OBJECT_DEPENDS ${fileIn}
				)
			SET(${out_var} "${fileOut}")
		else ()
			SET(${out_var} "")
			message (WARNING "Could not find windres, not compiling resource \"${fileIn}\".")
		endif ()
	endmacro ()
endif ()

