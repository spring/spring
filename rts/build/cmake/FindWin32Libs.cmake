# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Try to find some win32-only libraries needed to compile Spring
# Once done this will define
#
# WIN32_FOUND - System has the required libraries
# WIN32_LIBRARIES - Link these
#

SET(WIN32_FOUND TRUE)

if (MINGW)
	SET(WIN32_LIBRARY_SEARCHPATHS
		$ENV{MINGDIR}/lib)
	# no point in searching for these on a proper mingw installation
	SET(IMAGEHLP_LIBRARY -limagehlp)
	SET(IPHLPAPI_LIBRARY -liphlpapi)
	SET(WS2_32_LIBRARY -lws2_32)
	SET(WINMM_LIBRARY -lwinmm)
	SET(MINGW32_LIBRARY mingw32)
elseif (MSVC)
	SET(IMAGEHLP_LIBRARY imagehlp)
	SET(IPHLPAPI_LIBRARY iphlpapi)
	SET(WS2_32_LIBRARY ws2_32)
	SET(WINMM_LIBRARY winmm)
	SET(DBGHELP_LIBRARY dbghelp) #dont need for msvc
	SET(IMM32_LIBRARY imm32)
	SET(VERSION_LIBRARY version)
elseif (MINGW)
	SET(WIN32_LIBRARY_SEARCHPATHS
		/
		/usr/lib
		/usr/local/lib
		NO_DEFAULT_PATH)

	if (NOT IMAGEHLP_LIBRARY)
		FIND_LIBRARY(IMAGEHLP_LIBRARY imagehlp PATHS ${WIN32_LIBRARY_SEARCHPATHS})
		if (NOT IMAGEHLP_LIBRARY)
			message (SEND_ERROR "Could not find win32 IMAGEHLP library.")
			SET(WIN32_FOUND FALSE)
		endif ()
	endif ()

	if (NOT IPHLPAPI_LIBRARY)
		FIND_LIBRARY(IPHLPAPI_LIBRARY iphlpapi PATHS ${WIN32_LIBRARY_SEARCHPATHS})
		if (NOT IPHLPAPI_LIBRARY)
			message (SEND_ERROR "Could not find win32 IPHLPAPI library.")
			SET(WIN32_FOUND FALSE)
		endif ()
	endif ()

	if (NOT WS2_32_LIBRARY)
		FIND_LIBRARY(WS2_32_LIBRARY ws2_32 PATHS ${WIN32_LIBRARY_SEARCHPATHS})
		if (NOT WS2_32_LIBRARY)
			message (SEND_ERROR "Could not find win32 WS2_32 library.")
			SET(WIN32_FOUND FALSE)
		endif ()
	endif ()

	if (NOT WINMM_LIBRARY)
		FIND_LIBRARY(WINMM_LIBRARY winmm PATHS ${WIN32_LIBRARY_SEARCHPATHS})
		if (NOT WINMM_LIBRARY)
			message (SEND_ERROR "Could not find win32 WINMM library.")
			SET(WIN32_FOUND FALSE)
		endif ()
	endif ()
endif ()

if (WIN32_FOUND)
	SET(WIN32_LIBRARIES
		${DBGHELP_LIBRARY} # for stacktraces on msvc
		${IMAGEHLP_LIBRARY} # for System/Platform/Win/CrashHandler.cpp
		${IPHLPAPI_LIBRARY}
		${WS2_32_LIBRARY}	  # for System/Net/
		${WINMM_LIBRARY}
		${MINGW32_LIBRARY}
		${IMM32_LIBRARY} # for SDL2
		${VERSION_LIBRARY} # for SDL2
	)
	message (STATUS "Found win32 libraries: ${WIN32_LIBRARIES}")
endif ()

MARK_AS_ADVANCED(
	IMAGEHLP_LIBRARY
	IPHLPAPI_LIBRARY
	WS2_32_LIBRARY
	WINMM_LIBRARY
)
