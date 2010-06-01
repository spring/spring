
INCLUDE_DIRECTORIES(BEFORE ../)

set(lobbySources
		"Connection"
	)

add_library(lobby STATIC EXCLUDE_FROM_ALL ${lobbySources})
target_link_libraries(lobby md5 ${WIN32_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

add_executable(testapp EXCLUDE_FROM_ALL Test/testapp)
target_link_libraries(testapp lobby)

