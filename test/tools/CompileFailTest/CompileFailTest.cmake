
# @brief A macro for compile fail test (to test illegal code)
# based on Boost's BoostTesting.cmake
# URL: http://svn.boost.org/svn/boost/branches/CMake/Boost_1_35_0/tools/build/CMake/BoostTesting.cmake
macro(spring_test_compile_fail testname sourcefile def_flag)
	# Determine the include directories to pass along to the underlying project.
	Get_Directory_Property(TEST_INCLUDE_DIRS INCLUDE_DIRECTORIES)
	SET(TEST_INCLUDES "")
	FOREACH(DIR ${TEST_INCLUDE_DIRS})
		SET(TEST_INCLUDES "${TEST_INCLUDES}:${DIR}")
	ENDFOREACH(DIR ${TEST_INCLUDE_DIRS})

	# Add Test
	ADD_TEST("${testname}"
		${CMAKE_CTEST_COMMAND}
		--build-and-test
		"${CMAKE_CURRENT_SOURCE_DIR}/tools/CompileFailTest"
		"${CMAKE_CURRENT_BINARY_DIR}/tools/CompileFailTest"
		--build-generator "${CMAKE_GENERATOR}"
		--build-makeprogram "${CMAKE_MAKE_PROGRAM}"
		--build-project test
		--build-options -DSOURCE=${sourcefile} -DINCLUDES=${TEST_INCLUDES} -DDEF_FLAG="${def_flag}"
	)
	Set_Tests_Properties("${testname}" PROPERTIES WILL_FAIL ON)
endmacro(spring_test_compile_fail)
