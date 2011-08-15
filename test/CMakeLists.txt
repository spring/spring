# See README.md for usage instructions

FIND_PACKAGE(Boost 1.35.0 COMPONENTS unit_test_framework)
If    (NOT Boost_FOUND)
	Message(STATUS "Note: Unit tests will not be built: Boost::test library was not found")
Else  (NOT Boost_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	SET(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/rts")
	INCLUDE_DIRECTORIES(${ENGINE_SOURCE_DIR})

	add_definitions(-DBOOST_TEST_DYN_LINK -DSYNCCHECK)

	Set(test_Log_sources
			"${ENGINE_SOURCE_DIR}/System/Log/Backend.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/LogUtil.c"
			"${ENGINE_SOURCE_DIR}/System/Log/DefaultFilter.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/DefaultFormatter.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/ConsoleSink.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/StreamSink.cpp"
		)

	Add_Custom_Target(tests)



################################################################################
### UDPListener

	Set(test_UDPListener_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Net/TestUDPListener.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/UDPListener.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/RawPacket.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/PackPacket.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/ProtocolDef.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/UDPConnection.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/Connection.cpp"
			"${ENGINE_SOURCE_DIR}/System/Net/Socket.cpp"
			"${ENGINE_SOURCE_DIR}/System/CRC.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/NullGlobalConfig.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Nullerrorhandler.cpp"
			${test_Log_sources}
		)

	ADD_EXECUTABLE(test_UDPListener ${test_UDPListener_src})
	TARGET_LINK_LIBRARIES(test_UDPListener
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_SYSTEM_LIBRARY}
			${SDL_LIBRARY}
			7zip
		)

	ADD_TEST(NAME testUDPListener COMMAND test_UDPListener)
	Add_Dependencies(tests test_UDPListener)



################################################################################
### ILog

	Set(test_ILog_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Log/TestILog.cpp"
			${test_Log_sources}
		)

	ADD_EXECUTABLE(test_ILog ${test_ILog_src})
	TARGET_LINK_LIBRARIES(test_ILog
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	ADD_TEST(NAME testILog COMMAND test_ILog)
	Add_Dependencies(tests test_ILog)



################################################################################
### SyncedPrimitive

	Set(test_SyncedPrimitive_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Sync/TestSyncedPrimitive.cpp"
			"${ENGINE_SOURCE_DIR}/System/Sync/SyncChecker.cpp"
		)

	ADD_EXECUTABLE(test_SyncedPrimitive ${test_SyncedPrimitive_src})
	TARGET_LINK_LIBRARIES(test_SyncedPrimitive
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	ADD_TEST(NAME testSyncedPrimitive COMMAND test_SyncedPrimitive)
	Add_Dependencies(tests test_SyncedPrimitive)


################################################################################
### RectangleOptimizer

	Set(test_RectangleOptimizer_src
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Misc/RectangleOptimizer.cpp"
			"${ENGINE_SOURCE_DIR}/System/Misc/RectangleOptimizer.cpp"
			${test_Log_sources}
		)

	ADD_EXECUTABLE(test_RectangleOptimizer ${test_RectangleOptimizer_src})
	TARGET_LINK_LIBRARIES(test_RectangleOptimizer
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)

	ADD_TEST(NAME testRectangleOptimizer COMMAND test_RectangleOptimizer)
	Add_Dependencies(tests test_RectangleOptimizer)



################################################################################
### FileSystem

	Set(test_FileSystem_src
			"${ENGINE_SOURCE_DIR}/System/FileSystem/FileSystem.cpp"
			"${ENGINE_SOURCE_DIR}/System/FileSystem/FileSystemAbstraction.cpp"
			"${ENGINE_SOURCE_DIR}/System/Util.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/FileSystem/TestFileSystem.cpp"
			${test_Log_sources}
		)

	ADD_EXECUTABLE(test_FileSystem ${test_FileSystem_src})
	TARGET_LINK_LIBRARIES(test_FileSystem
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_REGEX_LIBRARY}
		)

	ADD_TEST(NAME testFileSystem COMMAND test_FileSystem)
	Add_Dependencies(tests test_FileSystem)



################################################################################


EndIf (NOT Boost_FOUND)
