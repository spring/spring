
# How does it work?
# 1. We run the `ctest` program and ask it run "${CMAKE_CURRENT_SOURCE_DIR}/tools/CompileFailTest/CMakeLists.txt"
#    with SOURCE=${sourcefile}.
# 2. CMake will then create a corresponding Makefile to compile ${sourcefile}.
# 3. `ctest` will then run this Makefile, the compilation should fail now (that's what we want to test)
# 4. with Set_Tests_Properties("${testname}" PROPERTIES WILL_FAIL ON)
#    we told CTest (not the program! the current CMakeFile build target!) that we want the test named "${testname}"
#    to fail, so it gets marked as "Passed" in summary

# @brief A macro for compile fail test (to test illegal code)
# based on Boost's BoostTesting.cmake
# URL: http://svn.boost.org/svn/boost/branches/CMake/Boost_1_35_0/tools/build/CMake/BoostTesting.cmake
macro (spring_test_compile_fail testname sourcefile def_flag)
	# Determine the include directories to pass along to the underlying project.
	get_directory_property(TEST_INCLUDE_DIRS INCLUDE_DIRECTORIES)
	set(TEST_INCLUDES "")
	foreach (DIR ${TEST_INCLUDE_DIRS})
		SET(TEST_INCLUDES "${TEST_INCLUDES}:${DIR}")
	endforeach ()

	# Add Test
	ADD_TEST("${testname}"
		${CMAKE_CTEST_COMMAND}
		--build-and-test
		"${CMAKE_CURRENT_SOURCE_DIR}/tools/CompileFailTest"
		"${CMAKE_CURRENT_BINARY_DIR}/tools/CompileFailTest"
		--build-generator "${CMAKE_GENERATOR}"
		--build-makeprogram "${CMAKE_MAKE_PROGRAM}"
		--build-project CompileFailTest
		--build-options -DSOURCE=${sourcefile} -DINCLUDES=${TEST_INCLUDES} -DDEF_FLAG="${def_flag}"
	)
	set_tests_properties("${testname}" PROPERTIES WILL_FAIL ON)
endmacro ()
