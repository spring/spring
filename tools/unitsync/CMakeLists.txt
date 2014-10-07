# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

FIND_PACKAGE_STATIC(DevIL REQUIRED)
FIND_PACKAGE_STATIC(ZLIB REQUIRED)

LIST(APPEND unitsync_libs ${IL_IL_LIBRARY} ${JPEG_LIBRARY} ${PNG_LIBRARY} ${TIFF_LIBRARY} ${GIF_LIBRARY})
LIST(APPEND unitsync_libs ${CMAKE_DL_LIBS})
LIST(APPEND unitsync_libs ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
LIST(APPEND unitsync_libs 7zip lua headlessStubs archives)
LIST(APPEND unitsync_libs ${ZLIB_LIBRARY})
LIST(APPEND unitsync_libs ${SPRING_MINIZIP_LIBRARY})

IF (WIN32)
	LIST(APPEND unitsync_libs ${WINMM_LIBRARY})
ENDIF (WIN32)

IF    (MINGW)
	# We still need these header files,
	# even if we are not going to link with SDL.
	# We have them available anyway (mingwlibs).
	FIND_PACKAGE(SDL2 REQUIRED)
	INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR})
ELSE  (MINGW)
	# Use a direct copy of the GL and SDL headers,
	# as these may not be available on headless systems.
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/SDL2)
ENDIF (MINGW)


ADD_DEFINITIONS(-DUNITSYNC)
ADD_DEFINITIONS(${PIC_FLAG})
ADD_DEFINITIONS(-DNOT_USING_CREG)
ADD_DEFINITIONS(-DHEADLESS)
ADD_DEFINITIONS(-DNO_SOUND)
ADD_DEFINITIONS(-DBITMAP_NO_OPENGL)
REMOVE_DEFINITIONS(-DTHREADPOOL)

set(ENGINE_SRC_ROOT "../../rts")

INCLUDE_DIRECTORIES(${DEVIL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SPRING_MINIZIP_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT}/lib/lua/include)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT}/lib/7zip)
INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT})
Include_Directories(${CMAKE_BINARY_DIR}/src-generated/engine)


set(main_files
	"${ENGINE_SRC_ROOT}/System/Config/ConfigHandler.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigLocater.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigSource.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigVariable.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigLocater.cpp"
	"${ENGINE_SRC_ROOT}/System/CRC.cpp"
	"${ENGINE_SRC_ROOT}/System/Misc/SpringTime.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/CpuID.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/Misc.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/ScopedFileLock.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/Threading.cpp"
	"${ENGINE_SRC_ROOT}/System/LogOutput.cpp"
	"${ENGINE_SRC_ROOT}/System/TdfParser.cpp"
	"${ENGINE_SRC_ROOT}/System/ThreadPool.cpp"
	"${ENGINE_SRC_ROOT}/System/Info.cpp"
	"${ENGINE_SRC_ROOT}/System/Option.cpp"
	"${ENGINE_SRC_ROOT}/System/SafeVector.cpp"
	"${ENGINE_SRC_ROOT}/System/SafeCStrings.c"
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
	${sources_engine_System_Log_sinkFile}
	${sources_engine_System_Log_sinkOutputDebugString}
	${main_files}
	${CMAKE_CURRENT_SOURCE_DIR}/unitsync.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LuaParserAPI.cpp
	)

# HACK unitsync should actually be MODULE (not SHARED),
#   but the python bindings need it as SHARED,
#   to be able to link to it
ADD_LIBRARY(unitsync SHARED ${unitsync_files})
Add_Dependencies(unitsync generateVersionFiles)
TARGET_LINK_LIBRARIES(unitsync ${unitsync_libs})
if (MINGW)
	set_target_properties(unitsync PROPERTIES LINK_FLAGS -Wl,--add-stdcall-alias)
endif (MINGW)
FixLibName(unitsync)

install (TARGETS unitsync DESTINATION ${LIBDIR})

Add_Subdirectory(test)

OPTION(UNITSYNC_PYTHON_BINDINGS "compile python bindings for unitsync (FIXME: broken with gcc 4.9 see #4377)" OFF)
If     (UNITSYNC_PYTHON_BINDINGS)
	# only add this if the submodule is present
	Add_Subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/python")
EndIf()
