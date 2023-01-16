add_library(engineSystemNet STATIC EXCLUDE_FROM_ALL
	LocalConnection.cpp
	LoopbackConnection.cpp
	PackPacket.cpp
	ProtocolDef.cpp
	RawPacket.cpp
	Socket.cpp
	UDPConnection.cpp
	UDPListener.cpp
	UnpackPacket.cpp
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
