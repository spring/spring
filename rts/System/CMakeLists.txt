
# This list was created using this *nix shell command:
# > find . -name "*.cpp" -or -name "*.c" | sort
# Then Sound/ stuff was removed, because it is now a separate static lib.
make_global_var(sources_engine_System_common
	"${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfig.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Input/MouseInput.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Platform/errorhandler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SafeVector.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SplashScreen.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SpringApp.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Threading/ThreadPool.cpp"
)
if    (NO_CREG)
	make_global_var(sources_engine_System_creg )
else ()
	make_global_var(sources_engine_System_creg
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/SerializeLuaState.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/Serializer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/VarTypes.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/creg.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/creg/creg_runtime_tests.cpp"
	)
endif ()
make_global_var(sources_engine_System_FileSystem
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/ArchiveNameResolver.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/ArchiveLoader.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/ArchiveScanner.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/CacheDir.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/DataDirLocater.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/DataDirsAccess.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileFilter.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileSystem.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileSystemAbstraction.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/FileSystemInitializer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/GZFileHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/RapidHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/SimpleParser.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/VFSHandler.cpp"
	)
make_global_var(sources_engine_System_Log
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/Backend.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/DefaultFilter.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/DefaultFormatter.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/FramePrefixer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/LogSinkHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/LogUtil.c"
	)
make_global_var(sources_engine_System_Log_sinkStream
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/StreamSink.cpp"
	)
make_global_var(sources_engine_System_Log_sinkConsole
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/ConsoleSink.cpp"
	)
make_global_var(sources_engine_System_Log_sinkFile
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/FileSink.cpp"
	)
make_global_var(sources_engine_System_Log_sinkOutputDebugString
		"${CMAKE_CURRENT_SOURCE_DIR}/Log/OutputDebugStringSink.cpp"
	)
set(sources_engine_System_Platform_Linux
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/CrashHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/SoLib.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/MessageBox.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/WindowManagerHelper.cpp"
	)
set(sources_engine_System_Platform_Mac
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/SoLib.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/MessageBox.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/CrashHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/WindowManagerHelper.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/ThreadSupport.cpp"
	)
set(sources_engine_System_Platform_Windows
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/CrashHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/DllLib.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/MessageBox.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/seh.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/WinVersion.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/WindowManagerHelper.cpp"
	)

set(sources_engine_System_Threading_Mac
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/Signal.cpp"
	)
set(sources_engine_System_Threading_Linux
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/Futex.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/ThreadSupport.cpp"
	)
set(sources_engine_System_Threading_Windows
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Win/CriticalSection.cpp"
	)


target_sources(platform_sources
	PRIVATE
		${sources_engine_System_Platform_Windows}
		${sources_engine_System_Platform_Linux}
		${sources_engine_System_Platform_Mac}
		${sources_engine_System_Threading_Windows}
		${sources_engine_System_Threading_Linux}
		${sources_engine_System_Threading_Mac}
	)

set(sources_engine_System_Threading
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/CpuID.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Platform/Threading.cpp"
	)

set(sources_engine_System
		${sources_engine_System_common}
		${sources_engine_System_creg}
		${sources_engine_System_FileSystem}
		${sources_engine_System_Log}
		${sources_engine_System_Log_sinkConsole}
		${sources_engine_System_Log_sinkFile}
	)


### only use the target platform related directory
if     (APPLE)
	make_global_var(sources_engine_System_Threading ${sources_engine_System_Threading} ${sources_engine_System_Threading_Mac})
	make_global_var(sources_engine_System
			${sources_engine_System}
			${sources_engine_System_Platform_Mac}
			${sources_engine_System_Threading}
		)
elseif (UNIX)
	make_global_var(sources_engine_System_Threading ${sources_engine_System_Threading} ${sources_engine_System_Threading_Linux})
	make_global_var(sources_engine_System
			${sources_engine_System}
			${sources_engine_System_Platform_Linux}
			${sources_engine_System_Threading}
		)

elseif (WIN32)
	make_global_var(sources_engine_System_Threading ${sources_engine_System_Threading} ${sources_engine_System_Threading_Windows})
	make_global_var(sources_engine_System
			${sources_engine_System}
			${sources_engine_System_Log_sinkOutputDebugString}
			${sources_engine_System_Platform_Windows}
			${sources_engine_System_Threading}
		)
else ()
	make_global_var(sources_engine_System
			${sources_engine_System}
			${sources_engine_System_Threading}
		)
endif ()

add_subdirectory(BuildType)
add_subdirectory(FileSystem)
add_subdirectory(Net)

add_library(engineMisc STATIC
	LoadSave/CregLoadSaveHandler.cpp
	LoadSave/Demo.cpp
	LoadSave/DemoReader.cpp
	LoadSave/DemoRecorder.cpp
	LoadSave/LoadSaveHandler.cpp
	LoadSave/LuaLoadSaveHandler.cpp
	AIScriptHandler.cpp
	Color.cpp
	Config/ConfigHandler.cpp
	Config/ConfigLocater.cpp
	Config/ConfigSource.cpp
	Config/ConfigVariable.cpp
	CRC.cpp
	EventClient.cpp
	EventHandler.cpp
	Info.cpp
	Input/InputHandler.cpp
	Input/KeyInput.cpp
	LogOutput.cpp
	Main.cpp
	Matrix44f.cpp
	Misc/RectangleOverlapHandler.cpp
	Misc/SpringTime.cpp
	Object.cpp
	OffscreenGLContext.cpp
	Option.cpp
	Platform/Clipboard.cpp
	Platform/Misc.cpp
	Platform/SharedLib.cpp
	Platform/ScopedFileLock.cpp
	Platform/SDL1_keysym.cpp
	Platform/Watchdog.cpp
	Platform/WindowManagerHelper.cpp
	Rectangle.cpp
	SafeCStrings.c
	StartScriptGen.cpp
	Sync/DumpState.cpp
	Sync/FPUCheck.cpp
	Sync/Logger.cpp
	Sync/SHA512.cpp
	Sync/SyncChecker.cpp
	Sync/SyncDebugger.cpp
	Sync/SyncedFloat3.cpp
	Sync/backtrace.c
	Sync/get_executable_name.c
	TdfParser.cpp
	TimeProfiler.cpp
	TimeUtil.cpp
	UriParser.cpp
	StringHash.cpp
	type2.cpp
	float3.cpp
	float4.cpp
	SpringMath.cpp
)

target_include_directories(engineMisc
	PRIVATE
		${Spring_SOURCE_DIR}/rts
		${SDL2_INCLUDE_DIR}
		${Spring_SOURCE_DIR}/rts/lib/lua/include	
		${Spring_SOURCE_DIR}/tools/pr-downloader/src/lib/7z
		${Spring_SOURCE_DIR}/rts/lib/slimsig/include
		${GLEW_INCLUDE_DIR}
	PUBLIC
		${SPRING_MINIZIP_INCLUDE_DIR}
)

target_link_libraries(engineMisc
	PRIVATE
		7zip
)
