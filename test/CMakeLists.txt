# See README.md for usage instructions
If (MINGW)
	RemoveFlag("-mwindows")
EndIf (MINGW)

FIND_PACKAGE(Boost 1.35.0 COMPONENTS unit_test_framework)

If    (NOT Boost_FOUND)
	Message(STATUS "Note: Unit tests will not be built: Boost::test library was not found")
Else  (NOT Boost_FOUND)
	# defines spring_test_compile_fail macro
	INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/tools/CompileFailTest/CompileFailTest.cmake)

	IF    (UNIX AND (NOT APPLE) AND (NOT MINGW))
		FIND_LIBRARY(REALTIME_LIBRARY rt)

		IF    (PREFER_STATIC_LIBS AND NOT EXISTS "${REALTIME_LIBRARY}")
			MESSAGE(FATAL_ERROR "librt.[so|a] not found! Needed by std::chrono when statically linked!")
		ENDIF (PREFER_STATIC_LIBS AND NOT EXISTS "${REALTIME_LIBRARY}")
	ENDIF (UNIX AND (NOT APPLE) AND (NOT MINGW))

	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/src-generated/engine)

	SET(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/rts")
	INCLUDE_DIRECTORIES(${ENGINE_SOURCE_DIR})
	INCLUDE_DIRECTORIES(${ENGINE_SOURCE_DIR}/lib/asio/include)

	If	(NOT (WIN32 OR Boost_USE_STATIC_LIBS))
		#Win32 tests links static
		add_definitions(-DBOOST_TEST_DYN_LINK)
	EndIf	(NOT (WIN32 OR Boost_USE_STATIC_LIBS))

	add_definitions(-DSYNCCHECK -DUNIT_TEST)
	REMOVE_DEFINITIONS(-DTHREADPOOL)

	If (WIN32)
		EXECUTE_PROCESS(COMMAND ping -6 ::1 -n 1
			RESULT_VARIABLE IPV6_RET
			ERROR_QUIET)
	Else(WIN32)
		EXECUTE_PROCESS(COMMAND ping6 ::1 -c 1
			RESULT_VARIABLE IPV6_RET
			ERROR_QUIET)
	EndIf (WIN32)

	if (NOT ${IPV6_RET} EQUAL 0)
		add_definitions(-DNO_IPV6)
		Message(STATUS "No ipv6 support, disabling test")
	endif (NOT ${IPV6_RET} EQUAL 0)

	Set(test_Log_sources
			"${ENGINE_SOURCE_DIR}/System/SafeCStrings.c"
			"${ENGINE_SOURCE_DIR}/System/Log/Backend.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/LogUtil.c"
			"${ENGINE_SOURCE_DIR}/System/Log/DefaultFilter.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/DefaultFormatter.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/FramePrefixer.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/ConsoleSink.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/StreamSink.cpp"
		)

	Add_Custom_Target(tests)
	add_custom_target(check ${CMAKE_CTEST_COMMAND} --output-on-failure -V
		DEPENDS engine-headless)
	add_custom_target(install-tests)

	macro (add_spring_test target sources libraries flags)
		ADD_TEST(NAME test${target} COMMAND test_${target})
		add_dependencies(tests test_${target})
		add_dependencies(check test_${target})
		add_dependencies(install-tests test_${target})
		add_executable(test_${target} EXCLUDE_FROM_ALL ${sources})
		target_link_libraries(test_${target} ${libraries})
		set_target_properties(test_${target} PROPERTIES COMPILE_FLAGS "${flags}")
		#install(TARGETS test_${target} DESTINATION ${BINDIR})
	endmacro()

################################################################################
### UDPListener
# disabled for travis: https://springrts.com/mantis/view.php?id=5014
if(NOT DEFINED ENV{CI})
	set(test_name UDPListener)
	Set(test_src
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
		${test_Log_sources}
	)

	set(test_libs
		engineSystemNet
		${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		${REALTIME_LIBRARY}
		${WINMM_LIBRARY}
		${WS2_32_LIBRARY}
		7zip
	)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	Add_Dependencies(test_UDPListener generateVersionFiles)
endif()

################################################################################
### ILog
	set(test_name ILog)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Log/TestILog.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/FileSink.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/OutputDebugStringSink.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### SyncedPrimitive
	set(test_name SyncedPrimitive)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Sync/TestSyncedPrimitive.cpp"
			"${ENGINE_SOURCE_DIR}/System/Sync/SyncChecker.cpp"
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### RectangleOptimizer
	set(test_name RectangleOptimizer)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/RectangleOptimizer.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/RectangleOptimizer.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DNOT_USING_CREG")

################################################################################
### Float3
	set(test_name Float3)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testFloat3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")

################################################################################
### Matrix44f
	set(test_name Matrix44f)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testMatrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/Matrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")

################################################################################
### Matrix44fRotation
	set(test_name Matrix44fRotation)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testRotationMatrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/Matrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")

################################################################################
### SpringTime
	set(test_name SpringTime)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/testSpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${REALTIME_LIBRARY}
			${WINMM_LIBRARY}
		)

	set(test_flags "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")
	if (Boost_TIMER_FOUND)
		LIST(APPEND test_libs ${Boost_TIMER_LIBRARY})
		set(test_flags "${test_flags} -DBoost_TIMER_FOUND")
	endif (Boost_TIMER_FOUND)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")


################################################################################
### BitwiseEnum
	set(test_name BitwiseEnum)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/TestBitwiseEnum.cpp"
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	# positive tests (should compile fine)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	Add_Dependencies(tests test_BitwiseEnum)

	# negative tests (must not compile)
	spring_test_compile_fail(testBitwiseEnum_fail1 ${test_src} "-DTEST1")
	spring_test_compile_fail(testBitwiseEnum_fail2 ${test_src} "-DTEST2")
	spring_test_compile_fail(testBitwiseEnum_fail3 ${test_src} "-DTEST3")


################################################################################
### FileSystem
	set(test_name FileSystem)
	Set(test_src
			"${ENGINE_SOURCE_DIR}/System/FileSystem/FileSystem.cpp"
			"${ENGINE_SOURCE_DIR}/System/FileSystem/FileSystemAbstraction.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringUtil.cpp"
			"${ENGINE_SOURCE_DIR}/Game/GameVersion.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/FileSystem/TestFileSystem.cpp"
			${test_Log_sources}
		)
	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_FILESYSTEM_LIBRARY}
			${Boost_REGEX_LIBRARY}
		)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	add_dependencies(test_${test_name} generateVersionFiles)
################################################################################
### LuaSocketRestrictions
	set(test_name LuaSocketRestrictions)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/lib/luasocket/TestRestriction.cpp"
			"${ENGINE_SOURCE_DIR}/lib/luasocket/src/restrictions.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_REGEX_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DTEST")

	if    (NOT NO_CREG)
################################################################################
### CREG
		add_test(NAME testCreg COMMAND ${CMAKE_BINARY_DIR}/spring-headless${CMAKE_EXECUTABLE_SUFFIX} --test-creg)

### CREG LoadSave
		set(test_name LoadSave)
		Set(test_src
				"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/LoadSave/testCregLoadSave.cpp"
				"${ENGINE_SOURCE_DIR}/System/creg/Serializer.cpp"
				"${ENGINE_SOURCE_DIR}/System/creg/VarTypes.cpp"
				"${ENGINE_SOURCE_DIR}/System/creg/creg.cpp"
				${test_Log_sources}
			)

		set(test_libs
				${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
				${Boost_REGEX_LIBRARY}
			)

		add_spring_test(${test_name} "${test_src}" "${test_libs}" -"DTEST")
###
################################################################################
	endif (NOT NO_CREG)

################################################################################
### UnitSync
	set(test_name UnitSync)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/unitsync/testUnitSync.cpp"
			"${ENGINE_SOURCE_DIR}/Lua/LuaMemPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${CMAKE_DL_LIBS}
			unitsync
		)

	set(test_flags "-DUNITSYNC")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	add_dependencies(test_${test_name} springcontent.sdz)

################################################################################
### ThreadPool
	set(test_name ThreadPool)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testThreadPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Threading/ThreadPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/Platform/CpuID.cpp"
			"${ENGINE_SOURCE_DIR}/System/Platform/Threading.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
		)
add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DTHREADPOOL -DUNITSYNC")



################################################################################
### Mutex
	set(test_name Mutex)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/other/testMutex.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
			${WS2_32_LIBRARY}
		)

	set(test_flags "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### Ellipsoid
	set(test_name Ellipsoid)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/Sim/Misc/testEllipsoid.cpp"
			${test_Log_sources}
		)
	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)
	set(test_flags "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### QuadField
	set(test_name QuadField)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/Sim/Misc/testQuadField.cpp"
			"${ENGINE_SOURCE_DIR}/Sim/Misc/QuadField.cpp"
			${test_Log_sources}
		)
	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)
	set(test_flags "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### Printf
	set(test_name Printf)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/other/testPrintf.cpp"
			"${ENGINE_SOURCE_DIR}/Lua/LuaMemPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)
	set(test_libs
			streflop
			lua
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
			headlessStubs
		)
	set(test_flags "-DNOT_USING_CREG -DSTREFLOP_SSE -DBUILDING_AI")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	target_include_directories(test_${test_name} PRIVATE ${ENGINE_SOURCE_DIR}/lib/lua/include)


################################################################################
### SQRT
	set(test_name SQRT)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testSQRT.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/StringHash.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)
	set(test_libs
			streflop
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${WINMM_LIBRARY}
		)
	set(test_flags "-DNOT_USING_CREG -DSTREFLOP_SSE -DBUILDING_AI")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")

################################################################################
### EventClient
	set(test_name EventClient)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/EventClient.cpp"
			${test_Log_sources}
		)
	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### SerializeLuaState
	set(test_name SerializeLuaState)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testSerializeLuaState.cpp"
			"${ENGINE_SOURCE_DIR}/Lua/LuaMemPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/Serializer.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/VarTypes.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/SerializeLuaState.cpp"
			"${ENGINE_SOURCE_DIR}/System/creg/creg.cpp"
			${sources_engine_System_Threading}
			${test_Log_sources}
		)
	set(test_libs
			lua
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			headlessStubs
		)
	set(test_flags "-DNOT_USING_STREFLOP")
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
	target_include_directories(test_${test_name} PRIVATE ${ENGINE_SOURCE_DIR}/lib/lua/include)

################################################################################
EndIf (NOT Boost_FOUND)

add_subdirectory(headercheck)

