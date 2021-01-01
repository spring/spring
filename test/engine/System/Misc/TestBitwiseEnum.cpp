/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Misc/BitwiseEnum.h"

#include <stdlib.h>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

using namespace Bitwise;

namespace Colors { enum Colors {red, green, blue}; };
enum Vehicles {car, bus, train};
typedef BitwiseEnum<Colors::Colors> Color;

bool foo(Color c) { return true; }

// NOTE: these are compile time tests
#ifndef FAIL
TEST_CASE("ShouldCompile")
{
	Color c = Colors::red | Colors::green;

	CHECK(foo(c));
	CHECK(foo(Colors::red));
	CHECK(foo(Colors::red | Colors::green));

	// test overloaded global operator| (see http://code.google.com/p/mili/issues/detail?id=40)
	int x = Colors::red | Colors::green;

	// test operator=
	Color c2 = Colors::blue;
	c = c2;
	c = Colors::green;
}
#endif


#ifdef FAIL
TEST_CASE("MustNotCompile")
{
#ifdef TEST1
	CHECK(foo(Vehicles::car));
#endif

#ifdef TEST2
	CHECK(foo(1));
#endif

#ifdef TEST3
	Color c = Colors::blue;
	c = 1;
#endif
}
#endif
