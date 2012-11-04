set(SOURCE_ROOT ../../..)

include_directories(${SOURCE_ROOT})
include_directories(${SOURCE_ROOT}/lib)

add_definitions(${PIC_FLAG})
add_library(archives STATIC
	BufferedArchive.cpp
	DirArchive.cpp
	IArchive.cpp
	PoolArchive.cpp
	SevenZipArchive.cpp
	ZipArchive.cpp
	${sources_engine_System_Log}
	${sources_engine_System_Log_sinkConsole}
)

target_link_libraries(archives
	7zip
	${SPRING_MINIZIP_LIBRARY}
)
