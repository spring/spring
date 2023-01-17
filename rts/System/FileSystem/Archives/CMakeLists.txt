find_package(LibGit2)

set(archives_sources
	BufferedArchive.cpp
	DirArchive.cpp
	GitArchive.cpp
	IArchive.cpp
	PoolArchive.cpp
	SevenZipArchive.cpp
	VirtualArchive.cpp
	ZipArchive.cpp
)

add_library(archives STATIC EXCLUDE_FROM_ALL
	${archives_sources}
)

target_include_directories(archives
	PRIVATE
		${Spring_SOURCE_DIR}/rts
	PUBLIC
		${ZLIB_INCLUDE_DIR}
		${SPRING_MINIZIP_INCLUDE_DIR}
)

target_link_libraries(archives
	7zip
	${SPRING_MINIZIP_LIBRARY}
)
target_compile_options(archives PRIVATE ${PIC_FLAG})

if (LibGit2_FOUND)
	target_compile_definitions(archives PRIVATE LIBGIT2)
	target_link_libraries(archives ${GIT2_LIBRARY})
	target_include_directories(archives PRIVATE ${GIT2_INCLUDE_PATH})
endif()

