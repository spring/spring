# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

LIST(APPEND unitsync_libs ${DEVIL_IL_LIBRARY})
LIST(APPEND unitsync_libs ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})
LIST(APPEND unitsync_libs 7zip minizip lua)
LIST(APPEND unitsync_libs headlessStubs)

IF    (MINGW)
	# We still need these header files,
	# even if we are not going to link with SDL.
	# We have them available anyway (mingwlibs).
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
ELSE  (MINGW)
	# Use a direct copy of the GL and SDL headers,
	# as these may not be available on headless systems.
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/SDL)
ENDIF (MINGW)


ADD_DEFINITIONS(-DUNITSYNC)
ADD_DEFINITIONS(${PIC_FLAG})
ADD_DEFINITIONS(-DNOT_USING_CREG)
ADD_DEFINITIONS(-DHEADLESS)
ADD_DEFINITIONS(-DNO_SOUND)
ADD_DEFINITIONS(-DBITMAP_NO_OPENGL)

set(ENGINE_SRC_ROOT "../../rts")

INCLUDE_DIRECTORIES(${DEVIL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT}/lib/lua/include)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT}/lib/7zip)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT})
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT}/System)


set(main_files
	"${ENGINE_SRC_ROOT}/System/Config/ConfigHandler.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigLocater.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigSource.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigVariable.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigLocater.cpp"
	"${ENGINE_SRC_ROOT}/System/CRC.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/Misc.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/ScopedFileLock.cpp"
	"${ENGINE_SRC_ROOT}/System/LogOutput.cpp"
	"${ENGINE_SRC_ROOT}/System/TdfParser.cpp"
	"${ENGINE_SRC_ROOT}/System/Info.cpp"
	"${ENGINE_SRC_ROOT}/System/Option.cpp"
	"${ENGINE_SRC_ROOT}/System/Util.cpp"
	"${ENGINE_SRC_ROOT}/System/float4.cpp"
	"${ENGINE_SRC_ROOT}/Sim/Misc/SideParser.cpp"
	"${ENGINE_SRC_ROOT}/Game/GameVersion.cpp"
	"${ENGINE_SRC_ROOT}/ExternalAI/LuaAIImplHandler.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaParser.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaUtils.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaIO.cpp"
	"${ENGINE_SRC_ROOT}/Map/MapParser.cpp"
	"${ENGINE_SRC_ROOT}/Map/SMF/SMFMapFile.cpp"
	"${ENGINE_SRC_ROOT}/Rendering/Textures/Bitmap.cpp"
	)
if (WIN32)
	LIST(APPEND main_files "${ENGINE_SRC_ROOT}/System/Platform/Win/WinVersion.cpp")
endif (WIN32)

set(unitsync_files
	${sources_engine_System_FileSystem}
	${sources_engine_System_Log}
	${ENGINE_SRC_ROOT}/System/Log/LogOutputSink.cpp
	${main_files}
	${CMAKE_CURRENT_SOURCE_DIR}/unitsync.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LuaParserAPI.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Syncer.cpp
	)

ADD_LIBRARY(unitsync MODULE ${unitsync_files})
TARGET_LINK_LIBRARIES(unitsync ${unitsync_libs})
if    (MINGW)
	set_target_properties(unitsync PROPERTIES LINK_FLAGS -Wl,--add-stdcall-alias)
endif (MINGW)
FixLibName(unitsync)

install(TARGETS unitsync DESTINATION ${LIBDIR})

set(UNITSYNC_PYTHON_WRAPPER TRUE CACHE BOOL "Create a python wrapper for unitsync, if python is found on the system")
if    (UNITSYNC_PYTHON_WRAPPER)
	FIND_PACKAGE(PythonLibs)
	FIND_PACKAGE(PythonInterp)

	if    (PYTHONLIBS_FOUND AND PYTHONINTERP_FOUND)
		MESSAGE(STATUS "Python unitsync wrapper will be created")

		SET(pythonUnitsyncTarget    pyunitsync)
		SET(pythonWrapperGenerator "${CMAKE_CURRENT_SOURCE_DIR}/binding_generator.py")
		SET(pythonWrapperScript    "${CMAKE_CURRENT_BINARY_DIR}/generate_unitsync_python_wrapper.py")
		SET(pythonWrapperCpp       "${CMAKE_CURRENT_BINARY_DIR}/unitsync_python_wrapper.cc")
		SET_SOURCE_FILES_PROPERTIES("${pythonWrapperScript}" "${pythonWrapperCpp}" PROPERTIES GENERATED TRUE)

		ADD_CUSTOM_COMMAND(
			OUTPUT
				"${CMAKE_CURRENT_SOURCE_DIR}/unitsync_api.h"
				"${pythonWrapperScript}"
			COMMAND
				"${PYTHON_EXECUTABLE}" "${pythonWrapperGenerator}"
				"${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/rts"
			DEPENDS
				"${pythonWrapperGenerator}"
			WORKING_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}"
			COMMENT
				"  ${pythonUnitsyncTarget}: Generating the wrapper script" VERBATIM
			)

		ADD_CUSTOM_COMMAND(
			OUTPUT
				"${pythonWrapperCpp}"
			COMMAND
				"${PYTHON_EXECUTABLE}" "${pythonWrapperScript}"
				">" "${pythonWrapperCpp}"
			DEPENDS
				"${pythonWrapperScript}"
			WORKING_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}"
			COMMENT
				"  ${pythonUnitsyncTarget}: Generating the wrapper C++ sources" VERBATIM
			)

		# TODO move the whole unitsync python wrapper into a sub-dir
		INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}")

		ADD_LIBRARY(${pythonUnitsyncTarget} MODULE "${pythonWrapperCpp}" ${unitsync_files})
		SET_TARGET_PROPERTIES(${pythonUnitsyncTarget} PROPERTIES PREFIX "")
		TARGET_LINK_LIBRARIES(${pythonUnitsyncTarget} ${unitsync_libs})

		#INSTALL(TARGETS ${pythonUnitsyncTarget} DESTINATION ${LIBDIR})
	else  (PYTHONLIBS_FOUND AND PYTHONINTERP_FOUND)
		MESSAGE(STATUS "Python unitsync wrapper will NOT be created")
	endif (PYTHONLIBS_FOUND AND PYTHONINTERP_FOUND)
endif (UNITSYNC_PYTHON_WRAPPER)

Add_Subdirectory(test)
