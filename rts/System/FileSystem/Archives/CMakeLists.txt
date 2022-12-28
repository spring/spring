find_package(LibGit2)

set(archives_sources
	BufferedArchive.cpp
	DirArchive.cpp
	IArchive.cpp
	PoolArchive.cpp
	SevenZipArchive.cpp
	VirtualArchive.cpp
	ZipArchive.cpp
)

if (LibGit2_FOUND)
	list(APPEND archives_sources GitArchive.cpp)
else()
	message(WARNING "LibGit2 not found, .git Archives are not supported")
endif()

add_library(archives STATIC
	${archives_sources}
	${sources_engine_System_Log}
	${sources_engine_System_Log_sinkConsole}
)

target_include_directories(archives PRIVATE
	${Spring_SOURCE_DIR}/rts
	${SPRING_MINIZIP_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${GIT2_INCLUDE_PATH}
)

target_link_libraries(archives
	7zip
	${SPRING_MINIZIP_LIBRARY}
	${GIT2_LIBRARY}
)
if (LibGit2_FOUND)
	target_compile_definitions(archives PRIVATE LIBGIT2)
endif()

target_compile_options(archives PRIVATE ${PIC_FLAG})
