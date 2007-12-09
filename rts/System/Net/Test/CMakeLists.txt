PROJECT(UnitTester)
SET(CMAKE_CXX_FLAGS "-g -O1 -Wall")
INCLUDE_DIRECTORIES(../ /usr/include/SDL)

ADD_EXECUTABLE(UnitTest main ../Net ../Connection ../LocalConnection ../ProtocolDef ../RawPacket ../UDPConnection ../UDPHardlink ../UDPListener ../UDPSocket)
TARGET_LINK_LIBRARIES(UnitTest SDL boost_thread)
