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

	add_definitions(-DBOOST_TEST_DYN_LINK)

	Set(test_Log_sources
			"${ENGINE_SOURCE_DIR}/System/Log/Backend.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/LogUtil.c"
			"${ENGINE_SOURCE_DIR}/System/Log/DefaultFilter.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/DefaultFormatter.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/ConsoleSink.cpp"
			"${ENGINE_SOURCE_DIR}/System/Log/StreamSink.cpp"
		)



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
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/NullGlobalConfig.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/engine/System/Nullerrorhandler.cpp"
			${test_Log_sources}
		)

	ADD_EXECUTABLE(test_UDPListener ${test_UDPListener_src})
	TARGET_LINK_LIBRARIES(test_UDPListener
			${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
			${Boost_SYSTEM_LIBRARY}
			${SDL_LIBRARY}
		)

	ADD_TEST(NAME testUDPListener COMMAND test_UDPListener)



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



################################################################################



	Set(test_targets
			test_UDPListener
			test_ILog
		)

	Add_Custom_Target(tests)
	Add_Dependencies(tests ${test_targets})

EndIf (NOT Boost_FOUND)

