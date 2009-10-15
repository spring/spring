add_library(lobbycon Connection)
target_link_libraries(lobbycon ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

add_executable(testapp testapp)
target_link_libraries(testapp lobbycon)
