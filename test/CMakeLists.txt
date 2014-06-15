# See README.md for usage instructions

FIND_PACKAGE(Boost 1.35.0 COMPONENTS unit_test_framework)

If    (NOT Boost_FOUND)
	Message(STATUS "Note: Unit tests will not be built: Boost::test library was not found")
Else  (NOT Boost_FOUND)
	# defines spring_test_compile_fail macro
	INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/tools/CompileFailTest/CompileFailTest.cmake)


	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/src-generated/engine)

	SET(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/rts")
	INCLUDE_DIRECTORIES(${ENGINE_SOURCE_DIR})

	If	(NOT (WIN32 OR Boost_USE_STATIC_LIBS))
		#Win32 tests links static
		add_definitions(-DBOOST_TEST_DYN_LINK)
	EndIf	(NOT (WIN32 OR Boost_USE_STATIC_LIBS))
	add_definitions(-DSYNCCHECK -DUNIT_TEST)
	REMOVE_DEFINITIONS(-DTHREADPOOL)

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
		${test_Log_sources}
	)

	set(test_libs
		engineSystemNet
		${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		${Boost_SYSTEM_LIBRARY}
		${Boost_THREAD_LIBRARY}
		${Boost_CHRONO_LIBRARY_WITH_RT}
		${WINMM_LIBRARY}
		${WS2_32_LIBRARY}
		7zip
	)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	Add_Dependencies(test_UDPListener generateVersionFiles)

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
			${Boost_SYSTEM_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DNOT_USING_CREG")

################################################################################
### Float3
	set(test_name Float3)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testFloat3.cpp"
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_SYSTEM_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")

################################################################################
### Matrix44f
	set(test_name Matrix44f)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testMatrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/Matrix44f.cpp"
			"${ENGINE_SOURCE_DIR}/System/float3.cpp"
			"${ENGINE_SOURCE_DIR}/System/float4.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			"${ENGINE_SOURCE_DIR}/System/UnsyncedRNG.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_SYSTEM_LIBRARY}
			${Boost_THREAD_LIBRARY}
			${Boost_CHRONO_LIBRARY_WITH_RT}
			${WINMM_LIBRARY}
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DNOT_USING_CREG -DNOT_USING_STREFLOP -DBUILDING_AI")

################################################################################
### SpringTime
	set(test_name SpringTime)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/testSpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/TimeProfiler.cpp"
			"${ENGINE_SOURCE_DIR}/System/UnsyncedRNG.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_SYSTEM_LIBRARY}
			${Boost_CHRONO_LIBRARY_WITH_RT}
			${Boost_THREAD_LIBRARY}
		)

	IF (WIN32)
		LIST(APPEND test_libs ${WINMM_LIBRARY})
	ENDIF (WIN32)

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
			"${ENGINE_SOURCE_DIR}/System/Util.cpp"
			"${ENGINE_SOURCE_DIR}/Game/GameVersion.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/FileSystem/TestFileSystem.cpp"
			${test_Log_sources}
		)
	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_FILESYSTEM_LIBRARY}
			${Boost_SYSTEM_LIBRARY}
			${Boost_REGEX_LIBRARY}
		)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
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

################################################################################
### CREG
	add_test(NAME testCreg COMMAND ${CMAKE_BINARY_DIR}/spring-headless${CMAKE_EXECUTABLE_SUFFIX} --test-creg)
	add_dependencies(tests testCreg)
	add_dependencies(tests engine-headless)

################################################################################
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

################################################################################
### UnitSync
	set(test_name UnitSync)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/unitsync/testUnitSync.cpp"
			${test_Log_sources}
		)

	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${CMAKE_DL_LIBS}
			unitsync
		)

	add_spring_test(${test_name} "${test_src}" "${test_libs}" "")
	add_dependencies(test_${test_name} springcontent.sdz)

################################################################################
### ThreadPool
	set(test_name ThreadPool)
	Set(test_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/testThreadPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/ThreadPool.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/SpringTime.cpp"
			"${ENGINE_SOURCE_DIR}/System/UnsyncedRNG.cpp"
			"${ENGINE_SOURCE_DIR}/System/Platform/CpuID.cpp"
			"${ENGINE_SOURCE_DIR}/System/Platform/Threading.cpp"
			${test_Log_sources}
		)
	set(test_libs
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_THREAD_LIBRARY}
			${Boost_CHRONO_LIBRARY_WITH_RT}
			${Boost_SYSTEM_LIBRARY}
			${WINMM_LIBRARY}
		)
	add_spring_test(${test_name} "${test_src}" "${test_libs}" "-DTHREADPOOL -DUNITSYNC")

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
EndIf (NOT Boost_FOUND)

add_subdirectory(headercheck)

