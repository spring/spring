# check whether given python module exists

macro(CheckPythonModule module)
	execute_process( 
		COMMAND 
			${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/CheckPythonModule.py" "${module}"
		RESULT_VARIABLE 
			python_${module}_RESULT_VAR
		)
	IF( python_${module}_RESULT_VAR EQUAL 0 )
		set( python_${module}_FOUND 1 )
	ENDIF( python_${module}_RESULT_VAR EQUAL 0 )
endmacro(CheckPythonModule)
