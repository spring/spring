# Spring supplied CMake build file


IF (WIN32)
    SET(ipsubdir "win32")
ELSE (WIN32)
    SET(ipsubdir "posix")
ENDIF (WIN32)

SET(oscpackSources
		"OscOutboundPacketStream"
		"OscTypes"
		"OscPrintReceivedElements"
		"OscReceivedElements"
		"ip/IpEndpointName"
		"ip/${ipsubdir}/NetworkingUtils"
		"ip/${ipsubdir}/UdpSocket"
	)


INCLUDE_DIRECTORIES( ${OSCPACK_SRC_ROOT} )

ADD_LIBRARY(oscpack STATIC EXCLUDE_FROM_ALL ${oscpackSources})
IF (APPLE)
	SET_TARGET_PROPERTIES(oscpack PROPERTIES COMPILE_FLAGS -DOSC_HOST_BIG_ENDIAN)
ELSE (APPLE)
	SET_TARGET_PROPERTIES(oscpack PROPERTIES COMPILE_FLAGS -DOSC_HOST_LITTLE_ENDIAN)
ENDIF (APPLE)

