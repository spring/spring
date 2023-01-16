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

target_include_directories(engineSystemNet
	PRIVATE
		${Spring_SOURCE_DIR}/rts
	PUBLIC
		${Spring_SOURCE_DIR}/rts/lib/asio/include
)

if(PREFER_STATIC_LIBS)
	target_compile_definitions(engineSystemNet INTERFACE
		ASIO_DISABLE_GETADDRINFO
	)
endif()
