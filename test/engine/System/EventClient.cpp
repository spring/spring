/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <iostream>
#include <typeinfo>

#define BOOST_TEST_MODULE EventClient
#include <boost/test/unit_test.hpp>


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


BOOST_AUTO_TEST_CASE( EventClient )
{
	// Checks Pointer-to-Member-Functions (PMF)
	// used by CEventClient to detect if a virtual function is overriden
	// and so if the derived class wants the event

	//BOOST_CHECK(&A::Foo == &B::Foo); // undefined for virtual methods

	// old way using gcc's pmf casting extension (illegal in iso c++)
	//BOOST_CHECK(reinterpret_cast<void(*)()>(&A::Foo) != reinterpret_cast<void(*)()>(&B::Foo));

	// new way should work everywhere
	BOOST_CHECK(typeid(&A::Foo) != typeid(&B::Foo));
	BOOST_CHECK(typeid(&A::Foo) == typeid(&A::Foo));
}
