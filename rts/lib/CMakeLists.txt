INCLUDE_DIRECTORIES(streflop lua/include luabind)

AUX_SOURCE_DIRECTORY(lua/src lualibfiles)
ADD_LIBRARY(lua STATIC ${lualibfiles})
TARGET_LINK_LIBRARIES(lua streflop)

AUX_SOURCE_DIRECTORY(luabind/src luabindfiles)
ADD_LIBRARY(luabind STATIC ${luabindfiles})
TARGET_LINK_LIBRARIES(luabind lua)

AUX_SOURCE_DIRECTORY(7zip 7zipfiles)
ADD_LIBRARY(7zip STATIC ${7zipfiles})

AUX_SOURCE_DIRECTORY(hpiutil2 hpifiles)
ADD_LIBRARY(hpiutil2 STATIC ${hpifiles})
TARGET_LINK_LIBRARIES(hpiutil2 z)

IF (NOT MINGW)
	ADD_LIBRARY(minizip STATIC  minizip/unzip minizip/zip minizip/ioapi)
ELSE (NOT MINGW)
	ADD_LIBRARY(minizip STATIC  minizip/unzip minizip/zip minizip/iowin32 minizip/ioapi)
ENDIF (NOT MINGW)
TARGET_LINK_LIBRARIES(minizip z)

ADD_SUBDIRECTORY(streflop)