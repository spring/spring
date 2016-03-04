# Downloaded from: http://www-id.imag.fr/FLOWVR/manual/flowvr-suite-src/flowvr-render/cmake/
# License: GPL v2, http://www-id.imag.fr/FLOWVR/manual/flowvr-suite-src/flowvr-render/COPYING

# - Try to find GLEW
# Once done this will define:
#  GLEW_FOUND         - system has GLEW
#  GLEW_INCLUDE_DIR   - the GLEW include directory
#  GLEW_LIBRARIES     - Link these to use GLEW
#

INCLUDE(FindPackageHandleStandardArgs)

IF    (GLEW_INCLUDE_DIR)
	# Already in cache, be silent
	SET(GLEW_FIND_QUIETLY TRUE)
ENDIF (GLEW_INCLUDE_DIR)

FIND_PATH(GLEW_INCLUDE_DIR
	NAMES
		GL/glew.h
	PATHS
		$ENV{CPATH}
		/usr/include
		/usr/local/include
		${PROJECT_BINARY_DIR}/include
		${PROJECT_SOURCE_DIR}/include
	NO_DEFAULT_PATH
	)
FIND_PATH(GLEW_INCLUDE_DIR NAMES GL/glew.h)

FIND_LIBRARY(GLEW_LIBRARIES
	NAMES
		GLEW
		glew32s
		glew32sd
	PATHS
		$ENV{LD_LIBRARY_PATH}
		$ENV{LIBRARY_PATH}
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		${PROJECT_BINARY_DIR}/lib64
		${PROJECT_BINARY_DIR}/lib
		${PROJECT_SOURCE_DIR}/lib64
		${PROJECT_SOURCE_DIR}/lib
	NO_DEFAULT_PATH
	)
IF    (WIN32)
	IF(GLEW_STATIC)
		FIND_LIBRARY(GLEW_LIBRARIES NAMES GLEW glew32s)
	ELSE(GLEW_STATIC)
		FIND_LIBRARY(GLEW_LIBRARIES NAMES GLEW glew32)
	ENDIF(GLEW_STATIC)
ELSE  (WIN32)
	FIND_LIBRARY(GLEW_LIBRARIES NAMES GLEW)
ENDIF (WIN32)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEW DEFAULT_MSG GLEW_INCLUDE_DIR GLEW_LIBRARIES)

# show the GLEW_INCLUDE_DIR and GLEW_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(GLEW_INCLUDE_DIR GLEW_LIBRARIES)
