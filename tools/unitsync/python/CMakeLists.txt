set(UNITSYNC_PYTHON_WRAPPER TRUE CACHE BOOL "Create a python wrapper for unitsync, if python is found on the system")
if    (UNITSYNC_PYTHON_WRAPPER)
	FIND_PACKAGE(PythonLibs)
	FIND_PACKAGE(PythonInterp)
	INCLUDE(CheckPythonModule.cmake)
	CheckPythonModule(pybindgen)
	CheckPythonModule(pygccxml)

	if    (PYTHONLIBS_FOUND AND PYTHONINTERP_FOUND AND python_pybindgen_FOUND AND python_pygccxml_FOUND)
		MESSAGE(STATUS "Python unitsync wrapper will be created")

		SET(pythonUnitsyncTarget    pyunitsync)
		SET(pythonWrapperGenerator "${CMAKE_CURRENT_SOURCE_DIR}/binding_generator.py")
		SET(pythonWrapperTest      "${CMAKE_CURRENT_SOURCE_DIR}/binding_test.py")
		SET(pythonWrapperScript    "${CMAKE_CURRENT_BINARY_DIR}/generate_unitsync_python_wrapper.py")
		SET(pythonWrapperCpp       "${CMAKE_CURRENT_BINARY_DIR}/unitsync_python_wrapper.cc")
		SET_SOURCE_FILES_PROPERTIES("${pythonWrapperScript}" "${pythonWrapperCpp}" PROPERTIES GENERATED TRUE)

		ADD_CUSTOM_COMMAND(
			OUTPUT
				"${CMAKE_SOURCE_DIR}/tools/unitsync/unitsync_api.h"
				"${pythonWrapperScript}"
			COMMAND
				"${PYTHON_EXECUTABLE}" "${pythonWrapperGenerator}"
				"${CMAKE_SOURCE_DIR}/tools/unitsync" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/rts"
			DEPENDS
				"${pythonWrapperGenerator}"
			WORKING_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}"
			COMMENT
				"  ${pythonUnitsyncTarget}: Generating the wrapper script" VERBATIM
			)

		ADD_CUSTOM_COMMAND(
			OUTPUT
				"${pythonWrapperCpp}"
			COMMAND
				"${PYTHON_EXECUTABLE}" "${pythonWrapperScript}"
				">" "${pythonWrapperCpp}"
			DEPENDS
				"${pythonWrapperScript}"
			WORKING_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}"
			COMMENT
				"  ${pythonUnitsyncTarget}: Generating the wrapper C++ sources" VERBATIM
			)
		ADD_CUSTOM_TARGET( "test-${pythonUnitsyncTarget}" 
			COMMAND 
				"${PYTHON_EXECUTABLE}" "${pythonWrapperTest}"
			DEPENDS
				${pythonUnitsyncTarget}
			WORKING_DIRECTORY
				"${CMAKE_BINARY_DIR}"
			)
		ADD_CUSTOM_TARGET( "test-${pythonUnitsyncTarget}-all" 
			COMMAND 
				"${PYTHON_EXECUTABLE}" "${pythonWrapperTest}" "1"
			DEPENDS
				${pythonUnitsyncTarget}
			WORKING_DIRECTORY
				"${CMAKE_BINARY_DIR}"
			)

		INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}")
		
		#this part can prolly be done in a much more elegant and sane way..
		set(transformed_unitsync_files "" )
		FOREACH( file ${unitsync_files} )
			if(IS_ABSOLUTE ${file} )
				LIST(APPEND transformed_unitsync_files ${file} )
			else(IS_ABSOLUTE ${file} )
				if(EXISTS ../${file})
					LIST(APPEND transformed_unitsync_files ../${file} )
				elseif(EXISTS ../../${file})
					LIST(APPEND transformed_unitsync_files ../../${file} )
				endif(EXISTS ../${file})
			endif(IS_ABSOLUTE ${file} )
		ENDFOREACH( file ${unitsync_files} )
		
		ADD_LIBRARY(${pythonUnitsyncTarget} MODULE "${pythonWrapperCpp}" ${transformed_unitsync_files})
		SET_TARGET_PROPERTIES(${pythonUnitsyncTarget} PROPERTIES PREFIX "")
		TARGET_LINK_LIBRARIES(${pythonUnitsyncTarget} ${unitsync_libs})

		INSTALL(TARGETS ${pythonUnitsyncTarget} DESTINATION ${LIBDIR})
	else  (PYTHONLIBS_FOUND AND PYTHONINTERP_FOUND AND python_pybindgen_FOUND AND python_pygccxml_FOUND)
		MESSAGE(STATUS "Python unitsync wrapper will NOT be created")
		#output why?
	endif (PYTHONLIBS_FOUND AND PYTHONINTERP_FOUND AND python_pybindgen_FOUND AND python_pygccxml_FOUND)
endif (UNITSYNC_PYTHON_WRAPPER)
