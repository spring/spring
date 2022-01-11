# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

if (NOT PREFER_STATIC_LIBS)
	set(PREFER_STATIC_LIBS FALSE)
	set(WARN_STATIC_LINK_SWITCH TRUE CACHE BOOL "")
	mark_as_advanced(WARN_STATIC_LINK_SWITCH)
endif ()

set(PREFER_STATIC_LIBS ${PREFER_STATIC_LIBS} CACHE BOOL "Try to link as much as possible libraries statically")
if (PREFER_STATIC_LIBS)
	if (WARN_STATIC_LINK_SWITCH)
		message (FATAL_ERROR "You cannot toggle `static linked` once you run cmake! You have to use a cmake `toolchain` file to enable this flag!")
	endif ()

	message (STATUS "Prefer static-linking!")

	set(Boost_USE_STATIC_LIBS TRUE)
	mark_as_advanced(Boost_USE_STATIC_LIBS)

	set(ORIG_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
	mark_as_advanced(ORIG_FIND_LIBRARY_SUFFIXES)

	include(TestCXXAcceptsFlag)

	check_cxx_accepts_flag("-static-libgcc" HAVE_STATIC_LIBGCC_FLAG)
	if (HAVE_STATIC_LIBGCC_FLAG)
		set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    -static-libgcc")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc")
	endif ()

	check_cxx_accepts_flag("-static-libstdc++" HAVE_STATIC_LIBSTDCXX_FLAG)
	if (HAVE_STATIC_LIBSTDCXX_FLAG)
		set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    -static-libstdc++")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libstdc++")
	endif ()

	macro (PREFER_STATIC_LIBS)
		if (WIN32 OR MINGW)
			set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
		else ()
			set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
		endif ()
	endmacro ()

	macro (UNPREFER_STATIC_LIBS)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIG_FIND_LIBRARY_SUFFIXES})
	endmacro ()

	macro (FIND_PACKAGE_STATIC)
		prefer_static_libs()
		find_package(${ARGV0} ${ARGV1})
		unprefer_static_libs()
	endmacro ()
else ()
	macro (PREFER_STATIC_LIBS)
	endmacro ()

	macro (UNPREFER_STATIC_LIBS)
	endmacro ()

	macro (FIND_PACKAGE_STATIC)
		find_package(${ARGV0} ${ARGV1})
	endmacro ()
endif ()
