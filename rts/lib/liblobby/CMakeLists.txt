aux_source_directory(md5 md5files)
add_library(lobby STATIC Connection ${md5files})
target_link_libraries(lobby ${WIN32_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

add_executable(testapp EXCLUDE_FROM_ALL Test/testapp)
target_link_libraries(testapp lobby)

