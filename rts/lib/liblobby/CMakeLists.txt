add_library(lobby Connection)
target_link_libraries(lobby ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

add_executable(testapp testapp)
target_link_libraries(testapp lobby)
