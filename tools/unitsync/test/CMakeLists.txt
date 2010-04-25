
# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/tools/unitsync/test/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./lua2php ...
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../../..")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

REMOVE_DEFINITIONS(-DUNITSYNC)

# To allow linking to MODULE_LIBRARY targets
CMAKE_POLICY(SET CMP0001 OLD)
SET(CMAKE_BACKWARDS_COMPATIBILITY "2.2")

ADD_EXECUTABLE(lua2php EXCLUDE_FROM_ALL lua2php)
TARGET_LINK_LIBRARIES(lua2php unitsync)
ADD_DEPENDENCIES(lua2php unitsync)
#INSTALL(TARGETS lua2php DESTINATION ${BINDIR})

ADD_EXECUTABLE(unitsyncTest EXCLUDE_FROM_ALL test)
TARGET_LINK_LIBRARIES(unitsyncTest unitsync)
ADD_DEPENDENCIES(unitsyncTest unitsync)
#INSTALL(TARGETS unitsyncTest DESTINATION ${BINDIR})

