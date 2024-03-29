# See README.md for usage instructions
if (MINGW)
	remove_flag("-mwindows")
endif ()
# defines spring_test_compile_fail macro
include(${CMAKE_CURRENT_SOURCE_DIR}/tools/CompileFailTest/CompileFailTest.cmake)

if    (UNIX AND (NOT APPLE) AND (NOT MINGW) AND (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD"))
	find_library(REALTIME_LIBRARY rt)

	if    (PREFER_STATIC_LIBS AND NOT EXISTS "${REALTIME_LIBRARY}")
		message(FATAL_ERROR "librt.[so|a] not found! Needed by std::chrono when statically linked!")
	endif ()
else ()
	set(REALTIME_LIBRARY "")
endif ()


set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/rts")

remove_definitions(-DTHREADPOOL)

if (WIN32)
	execute_process(COMMAND ping -6 ::1 -n 1
		RESULT_VARIABLE IPV6_RET
		ERROR_QUIET)
else ()
	execute_process(COMMAND ping6 ::1 -c 1
		RESULT_VARIABLE IPV6_RET
		ERROR_QUIET)
endif ()

if (NOT ${IPV6_RET} EQUAL 0)
	add_definitions(-DNO_IPV6)
	message(STATUS "No ipv6 support, disabling test")
endif ()

add_library(test_Log STATIC EXCLUDE_FROM_ALL
	"${ENGINE_SOURCE_DIR}/System/SafeCStrings.c"
	"${ENGINE_SOURCE_DIR}/System/Log/Backend.cpp"
	"${ENGINE_SOURCE_DIR}/System/Log/LogUtil.c"
	"${ENGINE_SOURCE_DIR}/System/Log/DefaultFilter.cpp"
	"${ENGINE_SOURCE_DIR}/System/Log/DefaultFormatter.cpp"
	"${ENGINE_SOURCE_DIR}/System/Log/FramePrefixer.cpp"
	"${ENGINE_SOURCE_DIR}/System/Log/ConsoleSink.cpp"
	"${ENGINE_SOURCE_DIR}/System/Log/StreamSink.cpp"
)

target_include_directories(test_Log PRIVATE
	${ENGINE_SOURCE_DIR}
)

add_custom_target(tests)
add_custom_target(check ${CMAKE_CTEST_COMMAND} --output-on-failure -V
	DEPENDS engine-headless)
add_custom_target(install-tests)

macro (add_spring_test target sources libraries flags)
	add_test(NAME test${target} COMMAND test_${target})
	add_dependencies(tests test_${target})
	add_dependencies(check test_${target})
	add_dependencies(install-tests test_${target})
	add_executable(test_${target} EXCLUDE_FROM_ALL ${sources})
	target_link_libraries(test_${target} ${libraries})
	target_include_directories(test_${target} PRIVATE
		${ENGINE_SOURCE_DIR}
		${ENGINE_SOURCE_DIR}/lib/asio/include
		${CMAKE_SOURCE_DIR}/tools/pr-downloader/src/lib/7z
		${CMAKE_BINARY_DIR}/src-generated/engine
	)
	target_compile_definitions(test_${target} PRIVATE
		SYNCCHECK
		UNIT_TEST
		"${flags}"
	)
	#install(TARGETS test_${target} DESTINATION ${BINDIR})
endmacro()

################################################################################
### UDPListener
# disabled for travis: https://springrts.com/mantis/view.php?id=5014
if(NOT DEFINED ENV{CI})
	set(test_name UDPListener)
	set(test_src
		"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Net/TestUDPListener.cpp"
		"${ENGINE_SOURCE_DIR}/Game/GameVersion.cpp"
		"${ENGINE_SOURCE_DIR}/Net/Protocol/BaseNetProtocol.cpp"
		"${ENGINE_SOURCE_DIR}/System/CRC.cpp"
		"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
		## HACK:
		##   the engineSystemNet lib is compiled *without* -DUNIT_TEST
		##   it includes UDPConnection which depends on ConfigHandler
		##   in normal builds, but fails to link when used as part of
		##   UT so we compile it again
		"${ENGINE_SOURCE_DIR}/System/Net/UDPConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/NullGlobalConfig.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Nullerrorhandler.cpp"
		${sources_engine_System_Threading}
	)

	set(test_libs
		engineSystemNet
		${REALTIME_LIBRARY}
		${WINMM_LIBRARY}
		${WS2_32_LIBRARY}
		7zip
		test_Log
	)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	add_dependencies(test_UDPListener generateVersionFiles)
endif ()

################################################################################
### ILog
	set(test_name ILog)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Log/TestILog.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/FileSink.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/OutputDebugStringSink.cpp"
		)

	set(test_libs
			test_Log
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### SyncedPrimitive
	set(test_name SyncedPrimitive)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Sync/TestSyncedPrimitive.cpp"
			"${ENGINE_SOURCE_DIR}/System/Sync/SyncChecker.cpp"
		)

	set(test_libs
			""
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### RectangleOverlapHandler
	set(test_name RectangleOverlapHandler)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/RectangleOverlapHandler.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/RectangleOverlapHandler.cpp"
		)

	set(test_libs
			test_Log
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "NOT_USING_CREG")

################################################################################
### Float3
	set(test_name Float3)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testFloat3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
		)

	set(test_libs
			${WINMM_LIBRARY}
			test_Log
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "NOT_USING_CREG;NOT_USING_STREFLOP;BUILDING_AI")

################################################################################
### Matrix44f
	set(test_name Matrix44f)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testMatrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/Matrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			${sources_engine_System_Threading}
		)

	set(test_libs
			${WINMM_LIBRARY}
			test_Log
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "NOT_USING_CREG;NOT_USING_STREFLOP;BUILDING_AI")

################################################################################
### Matrix44fRotation
	set(test_name Matrix44fRotation)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testRotationMatrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/Matrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
		)

	set(test_libs
			${WINMM_LIBRARY}
			test_Log
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "NOT_USING_CREG;NOT_USING_STREFLOP;BUILDING_AI")

################################################################################
### SpringTime
	set(test_name SpringTime)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/testSpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
		)

	set(test_libs
			test_Log
			${REALTIME_LIBRARY}
			${WINMM_LIBRARY}
		)

	set(test_flags NOT_USING_CREG NOT_USING_STREFLOP BUILDING_AI)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")


################################################################################
### BitwiseEnum
	set(test_name BitwiseEnum)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/TestBitwiseEnum.cpp"
		)

	set(test_libs
			""
		)

	# positive tests (should compile fine)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	add_dependencies(tests test_BitwiseEnum)

	# negative tests (must not compile)
	spring_test_compile_fail(testBitwiseEnum_fail1 ${test_src} "TEST1")
	spring_test_compile_fail(testBitwiseEnum_fail2 ${test_src} "TEST2")
	spring_test_compile_fail(testBitwiseEnum_fail3 ${test_src} "TEST3")


################################################################################
### FileSystem
	set(test_name FileSystem)
	set(test_src
			"${ENGINE_SOURCE_DIR}/System/FileSystem/FileSystem.cpp"
			"${ENGINE_SOURCE_DIR}/System/FileSystem/FileSystemAbstraction.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringUtil.cpp"
			"${ENGINE_SOURCE_DIR}/Game/GameVersion.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/FileSystem/TestFileSystem.cpp"
		)
	set(test_libs
			test_Log
		)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	add_dependencies(test_${test_name} generateVersionFiles)
################################################################################
### LuaSocketRestrictions
	set(test_name LuaSocketRestrictions)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/lib/luasocket/TestRestriction.cpp"
			"${ENGINE_SOURCE_DIR}/lib/luasocket/src/restrictions.cpp"
		)

	set(test_libs
			test_Log
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "TEST")

	if    (NOT NO_CREG)
################################################################################
### CREG

add_test(NAME testCreg COMMAND ${CMAKE_BINARY_DIR}/spring-headless${CMAKE_EXECUTABLE_SUFFIX} --test-creg)
add_test(NAME testDedicated COMMAND ${CMAKE_BINARY_DIR}/spring-headless${CMAKE_EXECUTABLE_SUFFIX} --version)
add_test(NAME testEngine COMMAND ${CMAKE_BINARY_DIR}/spring${CMAKE_EXECUTABLE_SUFFIX} --version)

### CREG LoadSave
		set(test_name LoadSave)
		set(test_src
				"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/LoadSave/testCregLoadSave.cpp"
				"${ENGINE_SOURCE_DIR}/System/creg/Serializer.cpp"
				"${ENGINE_SOURCE_DIR}/System/creg/VarTypes.cpp"
				"${ENGINE_SOURCE_DIR}/System/creg/creg.cpp"
			)

		set(test_libs
				test_Log
			)

		add_spring_test(${test_name} "${test_src}" "${test_libs}" "TEST")
###
################################################################################
	endif ()

################################################################################
### UnitSync
	set(test_name UnitSync)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/unitsync/testUnitSync.cpp"
			"${ENGINE_SOURCE_DIR}/Lua/LuaMemPool.cpp"
			## -DUNITSYNC is not passed onto VFS code, which references globalConfig
			"${ENGINE_SOURCE_DIR}/System/GlobalConfig.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			${sources_engine_System_Threading}
		)

	set(test_libs
			${CMAKE_DL_LIBS}
			unitsync
			test_Log
			7zip
		)

	set(test_flags UNITSYNC)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	add_dependencies(test_${test_name} springcontent.sdz)

################################################################################
### ThreadPool
	set(test_name ThreadPool)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testThreadPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Threading/ThreadPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/Platform/CpuID.cpp"
			"${ENGINE_SOURCE_DIR}/System/Platform/Threading.cpp"
			${sources_engine_System_Threading}
		)

	set(test_libs
			${WINMM_LIBRARY}
			test_Log
		)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		list(APPEND test_libs atomic)
	endif ()
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DTHREADPOOL -DUNITSYNC")



################################################################################
### Mutex
	set(test_name Mutex)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/other/testMutex.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
		)

	set(test_libs
			${WINMM_LIBRARY}
			${WS2_32_LIBRARY}
			test_Log
		)

	set(test_flags NOT_USING_CREG NOT_USING_STREFLOP BUILDING_AI)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### Ellipsoid
	set(test_name Ellipsoid)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/Sim/Misc/testEllipsoid.cpp"
		)
	set(test_libs
			test_Log
		)
	set(test_flags NOT_USING_CREG NOT_USING_STREFLOP BUILDING_AI)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### QuadField
	set(test_name QuadField)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/Sim/Misc/testQuadField.cpp"
			"${ENGINE_SOURCE_DIR}/Sim/Misc/QuadField.cpp"
		)
	set(test_libs
			test_Log
		)
	set(test_flags NOT_USING_CREG NOT_USING_STREFLOP BUILDING_AI)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### Printf
	set(test_name Printf)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/other/testPrintf.cpp"
			"${ENGINE_SOURCE_DIR}/Lua/LuaMemPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
		)
	set(test_libs
			streflop
			lua
			${WINMM_LIBRARY}
			headlessStubs
			test_Log
		)
	set(test_flags NOT_USING_CREG STREFLOP_SSE BUILDING_AI)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	target_include_directories(test_${test_name} PRIVATE ${ENGINE_SOURCE_DIR}/lib/lua/include)

################################################################################
### OpenGL
	set(test_name GLContextCreation)
	set(test_src
		"${CMAKE_CURRENT_SOURCE_DIR}/other/testGLContextCreation.cpp"
	)
	set(test_libs
		${OPENGL_gl_LIBRARY}
		${GLEW_LINK_LIBRARIES}
		${SDL2_LINK_LIBRARIES}
		test_Log
	)
	set(test_flags NOT_USING_CREG STREFLOP_SSE BUILDING_AI)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	target_include_directories(test_${test_name} PRIVATE ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})

################################################################################
### SQRT
	set(test_name SQRT)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testSQRT.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
		)
	set(test_libs
			streflop
			${WINMM_LIBRARY}
			test_Log
		)
	set(test_flags NOT_USING_CREG STREFLOP_SSE BUILDING_AI)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### EventClient
	set(test_name EventClient)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/EventClient.cpp"
		)
	set(test_libs
			test_Log
		)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### SerializeLuaState
	set(test_name SerializeLuaState)
	set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testSerializeLuaState.cpp"
			"${ENGINE_SOURCE_DIR}/Lua/LuaMemPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/Serializer.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/VarTypes.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/SerializeLuaState.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/creg.cpp"
			${sources_engine_System_Threading}
		)
	set(test_libs
			lua
			headlessStubs
			test_Log
		)
	set(test_flags NOT_USING_STREFLOP)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	target_include_directories(test_${test_name} PRIVATE ${ENGINE_SOURCE_DIR}/lib/lua/include)

################################################################################


add_subdirectory(headercheck)
