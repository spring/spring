
INCLUDE_DIRECTORIES(BEFORE ../)

SET(lobbySources
		"RawTextMessage"
		"Connection"
	)

ADD_LIBRARY(lobby STATIC EXCLUDE_FROM_ALL ${lobbySources})
TARGET_LINK_LIBRARIES(lobby md5 streflop ${WIN32_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

ADD_SUBDIRECTORY(Test)

