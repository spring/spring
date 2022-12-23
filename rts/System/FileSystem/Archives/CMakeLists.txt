find_package(LibGit2 REQUIRED)

add_library(archives STATIC
	BufferedArchive.cpp
	DirArchive.cpp
	GitArchive.cpp
	IArchive.cpp
	PoolArchive.cpp
	SevenZipArchive.cpp
	VirtualArchive.cpp
	ZipArchive.cpp
	${sources_engine_System_Log}
	${sources_engine_System_Log_sinkConsole}
)

target_include_directories(archives PRIVATE
	../../..
	${SPRING_MINIZIP_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${GIT2_INCLUDE_PATH}
)

target_link_libraries(archives
	7zip
	${SPRING_MINIZIP_LIBRARY}
	${GIT2_LIBRARY}
)

target_compile_options(archives PRIVATE ${PIC_FLAG})
