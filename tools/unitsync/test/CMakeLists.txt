
# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/tools/unitsync/test/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./lua2php ...
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

remove_definitions(-DUNITSYNC)

add_executable(lua2php EXCLUDE_FROM_ALL lua2php.cpp)
target_link_libraries(lua2php unitsync ${CMAKE_DL_LIBS})
add_dependencies(lua2php unitsync)
#INSTALL(TARGETS lua2php DESTINATION ${BINDIR})

add_executable(unitsyncTest EXCLUDE_FROM_ALL test.cpp)
target_link_libraries(unitsyncTest unitsync ${CMAKE_DL_LIBS})
add_dependencies(unitsyncTest unitsync)
#INSTALL(TARGETS unitsyncTest DESTINATION ${BINDIR})
