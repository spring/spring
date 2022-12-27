
add_definitions(${PIC_FLAG})
add_library(archives STATIC
	BufferedArchive.cpp
	DirArchive.cpp
	IArchive.cpp
	PoolArchive.cpp
	SevenZipArchive.cpp
	VirtualArchive.cpp
	ZipArchive.cpp
	${sources_engine_System_Log}
	${sources_engine_System_Log_sinkConsole}
)

target_include_directories(archives PRIVATE
	${Spring_SOURCE_DIR}/rts
	${ZLIB_INCLUDE_DIR}
	${SPRING_MINIZIP_INCLUDE_DIR}
)
target_link_libraries(archives
	7zip
	${SPRING_MINIZIP_LIBRARY}
)
