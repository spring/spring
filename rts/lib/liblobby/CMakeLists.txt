add_library(lobby Connection)
target_link_libraries(lobby ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

add_executable(testapp EXCLUDE_FROM_ALL testapp)
target_link_libraries(testapp lobby)
