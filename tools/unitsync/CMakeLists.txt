# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

find_package_static(ZLIB REQUIRED)

list(APPEND unitsync_libs ${CMAKE_DL_LIBS})
list(APPEND unitsync_libs 7zip lua headlessStubs archives)
list(APPEND unitsync_libs ${ZLIB_LIBRARY})
list(APPEND unitsync_libs ${SPRING_MINIZIP_LIBRARY})

if (WIN32)
	list(APPEND unitsync_libs ${IPHLPAPI_LIBRARY})
	list(APPEND unitsync_libs ${WINMM_LIBRARY})
endif (WIN32)

if    (MINGW OR MSVC)
	# We still need these header files,
	# even if we are not going to link with SDL.
	# We have them available anyway (mingwlibs).
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})
else  (MINGW OR MSVC)
	# Use a direct copy of the GL and SDL headers,
	# as these may not be available on headless systems.
	include_directories(${CMAKE_SOURCE_DIR}/include/SDL2)
endif (MINGW OR MSVC)


add_definitions(-DUNITSYNC)
if (NOT WIN32)
	add_definitions(-fPIC)
	add_definitions(${PIC_FLAG})
endif()
add_definitions(-DNOT_USING_CREG)
add_definitions(-DHEADLESS)
add_definitions(-DNO_SOUND)
add_definitions(-DBITMAP_NO_OPENGL)
#add_definitions(-DENABLE_DEPRECATED_FUNCTIONS)
remove_definitions(-DTHREADPOOL)

set(ENGINE_SRC_ROOT "../../rts")

include_directories(${SPRING_MINIZIP_INCLUDE_DIR})
include_directories(${ENGINE_SRC_ROOT}/lib/lua/include)
include_directories(${ENGINE_SRC_ROOT}/lib/7zip)
include_directories(${ENGINE_SRC_ROOT})
include_directories(${CMAKE_BINARY_DIR}/src-generated/engine)


set(main_files
	"${ENGINE_SRC_ROOT}/ExternalAI/LuaAIImplHandler.cpp"
	"${ENGINE_SRC_ROOT}/Game/GameVersion.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaConstEngine.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaMemPool.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaParser.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaUtils.cpp"
	"${ENGINE_SRC_ROOT}/Lua/LuaIO.cpp"
	"${ENGINE_SRC_ROOT}/Map/MapParser.cpp"
	"${ENGINE_SRC_ROOT}/Map/SMF/SMFMapFile.cpp"
	"${ENGINE_SRC_ROOT}/Sim/Misc/SideParser.cpp"
	"${ENGINE_SRC_ROOT}/Sim/Units/CommandAI/Command.cpp" ## LuaUtils::ParseCommand*
	## -DUNITSYNC is not passed onto VFS code, which references globalConfig
	"${ENGINE_SRC_ROOT}/System/GlobalConfig.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigHandler.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigLocater.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigSource.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigVariable.cpp"
	"${ENGINE_SRC_ROOT}/System/Config/ConfigLocater.cpp"
	"${ENGINE_SRC_ROOT}/System/Misc/SpringTime.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/CpuID.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/Misc.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/ScopedFileLock.cpp"
	"${ENGINE_SRC_ROOT}/System/Platform/Threading.cpp"
	"${ENGINE_SRC_ROOT}/System/Threading/ThreadPool.cpp"
	"${ENGINE_SRC_ROOT}/System/Sync/SHA512.cpp"
	"${ENGINE_SRC_ROOT}/System/CRC.cpp"
	"${ENGINE_SRC_ROOT}/System/float4.cpp"
	"${ENGINE_SRC_ROOT}/System/Info.cpp"
	"${ENGINE_SRC_ROOT}/System/LogOutput.cpp"
	"${ENGINE_SRC_ROOT}/System/Option.cpp"
	"${ENGINE_SRC_ROOT}/System/SafeVector.cpp"
	"${ENGINE_SRC_ROOT}/System/SafeCStrings.c"
	"${ENGINE_SRC_ROOT}/System/TdfParser.cpp"
	"${ENGINE_SRC_ROOT}/System/UriParser.cpp"
	"${ENGINE_SRC_ROOT}/System/StringUtil.cpp"
	)
if (WIN32)
	list(APPEND main_files "${ENGINE_SRC_ROOT}/System/Platform/Win/WinVersion.cpp")
else (WIN32)
        list(APPEND main_files "${ENGINE_SRC_ROOT}/System/Platform/Linux/ThreadSupport.cpp")
endif (WIN32)

set(unitsync_files
	${sources_engine_System_FileSystem}
	${sources_engine_System_Threading}
	${sources_engine_System_Log}
	${sources_engine_System_Log_sinkFile}
	${sources_engine_System_Log_sinkOutputDebugString}
	${main_files}
	${CMAKE_CURRENT_SOURCE_DIR}/unitsync.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LuaParserAPI.cpp
	)

# Add list of all exported functions to .def file to prevent decoration
if (MSVC)
	list(APPEND unitsync_files ${CMAKE_CURRENT_SOURCE_DIR}/exports.def)
endif ()

# HACK unitsync should actually be MODULE (not SHARED),
#   but the python bindings need it as SHARED,
#   to be able to link to it
add_library(unitsync SHARED ${unitsync_files})
add_dependencies(unitsync generateVersionFiles)
target_link_libraries(unitsync ${unitsync_libs})
if (MINGW)
	set_target_properties(unitsync PROPERTIES LINK_FLAGS -Wl,--add-stdcall-alias)
endif (MINGW)
fix_lib_name(unitsync)

install (TARGETS unitsync
	RUNTIME DESTINATION ${LIBDIR}
	LIBRARY DESTINATION ${LIBDIR})

add_subdirectory(test)

option(UNITSYNC_PYTHON_BINDINGS "compile python bindings for unitsync (FIXME: broken with gcc 4.9 see #4377)" OFF)
if (UNITSYNC_PYTHON_BINDINGS)
	# only add this if the submodule is present
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/python")
endif()
