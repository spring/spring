PROJECT(libnet CXX)
SET(CMAKE_CXX_FLAGS "-g -O1 -Wall -I../")
ADD_LIBRARY(net SHARED ../Net ../Connection ../LocalConnection ../ProtocolDef ../RawPacket ../TCPConnection ../TCPListener ../TCPSocket ../UDPConnection ../UDPHardlink ../UDPListener ../UDPSocket)
TARGET_LINK_LIBRARIES(net SDL boost_thread)

ADD_EXECUTABLE(UnitTest UnitTest)
TARGET_LINK_LIBRARIES(UnitTest net)
SET_TARGET_PROPERTIES(UnitTest PROPERTIES LINKER_LANGUAGE CXX)
