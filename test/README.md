# Spring RTS game engine - Unit Tests

## README

These are [Unit Tests](http://en.wikipedia.org/wiki/Unit_tests) for the Spring
project. They are useful for automatically testing the engine source code for
regressions.

Documentation about Boost unit tests can be found here:
* [writing - checks](http://www.boost.org/doc/libs/1_35_0/libs/test/doc/components/test_tools/reference/index.html)
* [writing - examples](http://www.boost.org/doc/libs/1_46_1/libs/test/doc/html/utf/testing-tools/output-test.html)
* [writing - examples (from IBM)](http://www.ibm.com/developerworks/aix/library/au-ctools1_boost/index.html?ca=drs-)
* [executing](http://www.boost.org/doc/libs/1_39_0/libs/test/doc/html/utf/user-guide/runtime-config/reference.html)

### Compiling

To compile all unit-test-suites:

	cmake .
	make tests

### Using

To run all unit-test-suites:

	make test

