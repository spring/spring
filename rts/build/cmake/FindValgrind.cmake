# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find the Valgrind library
#  VALGRIND_FOUND       - True if Valgrind was found.

Include(FindPackageHandleStandardArgs)

if (VALGRIND_INCLUDE_DIR)
	# Already in cache, be silent
	Set(VALGRIND_FIND_QUIETLY TRUE)
endif ()

Find_Path(VALGRIND_INCLUDE_DIR valgrind/valgrind.h)

if (VALGRIND_INCLUDE_DIR)
	set(VALGRIND_FOUND TRUE)
else ()
	set(VALGRIND_FOUND FALSE)
endif ()

Mark_As_Advanced(VALGRIND_INCLUDE_DIR)
