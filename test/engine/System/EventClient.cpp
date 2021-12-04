/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <iostream>
#include <typeinfo>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"


class A
{
public:
       virtual void Foo() {}
};



class B : public A
{
public:
       void Foo() {}
};


TEST_CASE("EventClient")
{
	// Checks Pointer-to-Member-Functions (PMF)
	// used by CEventClient to detect if a virtual function is overriden
	// and so if the derived class wants the event

	//CHECK(&A::Foo == &B::Foo); // undefined for virtual methods

	// old way using gcc's pmf casting extension (illegal in iso c++)
	//CHECK(reinterpret_cast<void(*)()>(&A::Foo) != reinterpret_cast<void(*)()>(&B::Foo));

	// new way should work everywhere
	CHECK(typeid(&A::Foo) != typeid(&B::Foo));
	CHECK(typeid(&A::Foo) == typeid(&A::Foo));
}
