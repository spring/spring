include_directories(${Spring_SOURCE_DIR}/rts/lib/asio/include)
include_directories(${Spring_SOURCE_DIR}/rts)
add_library(engineSystemNet STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/LocalConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/LoopbackConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/PackPacket.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/ProtocolDef.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/RawPacket.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Socket.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UDPConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UDPListener.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UnpackPacket.cpp"
	)

