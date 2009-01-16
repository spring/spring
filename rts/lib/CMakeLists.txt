INCLUDE_DIRECTORIES(BEFORE streflop lua/include luabind ${CMAKE_SOURCE_DIR}/rts/System)

ADD_DEFINITIONS(${PIC_FLAG}) # all libraries need to be position independent (gives compiler warnings on 64bit-linux

AUX_SOURCE_DIRECTORY(lua/src lualibfiles)
ADD_LIBRARY(lua STATIC ${lualibfiles})
TARGET_LINK_LIBRARIES(lua streflop)
if (UNIX)
	SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS -DLUA_USE_MKSTEMP)
endif (UNIX)

ADD_DEFINITIONS(-D_SZ_ONE_DIRECTORY)
AUX_SOURCE_DIRECTORY(7zip 7zipfiles)
ADD_LIBRARY(7zip STATIC EXCLUDE_FROM_ALL ${7zipfiles})

AUX_SOURCE_DIRECTORY(hpiutil2 hpifiles)
ADD_LIBRARY(hpiutil2 STATIC EXCLUDE_FROM_ALL ${hpifiles})
TARGET_LINK_LIBRARIES(hpiutil2 z)

AUX_SOURCE_DIRECTORY(gml gmlfiles)
ADD_LIBRARY(gml STATIC EXCLUDE_FROM_ALL ${gmlfiles})
if (MINGW)
	TARGET_LINK_LIBRARIES(gml opengl32 glu32)
else (MINGW)
	TARGET_LINK_LIBRARIES(gml GL GLU)
endif (MINGW)

IF (UNIX)
	ADD_LIBRARY(minizip STATIC EXCLUDE_FROM_ALL minizip/unzip minizip/zip minizip/ioapi)
ELSE (UNIX)
	ADD_LIBRARY(minizip STATIC EXCLUDE_FROM_ALL minizip/unzip minizip/zip minizip/iowin32 minizip/ioapi)
ENDIF (UNIX)
TARGET_LINK_LIBRARIES(minizip z)

ADD_SUBDIRECTORY(streflop)
