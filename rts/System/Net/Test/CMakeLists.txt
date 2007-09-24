PROJECT(libnet CXX)
SET(CMAKE_CXX_FLAGS "-g -O1 -Wall")
INCLUDE_DIRECTORIES(../ /usr/include/SDL)

ADD_EXECUTABLE(UnitTest UnitTest ../Net ../Connection ../LocalConnection ../ProtocolDef ../RawPacket ../TCPConnection ../TCPListener ../TCPSocket ../UDPConnection ../UDPHardlink ../UDPListener ../UDPSocket)
TARGET_LINK_LIBRARIES(UnitTest SDL boost_thread)
