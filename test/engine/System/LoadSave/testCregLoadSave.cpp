/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/creg_cond.h"
#include "System/creg/Serializer.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>

#define BOOST_TEST_MODULE CregLoadSave
#include <boost/test/unit_test.hpp>



class EmbeddedObj {
	CR_DECLARE(EmbeddedObj);
	int value;
};

CR_BIND(EmbeddedObj, );
CR_REG_METADATA(EmbeddedObj, CR_MEMBER(value));

enum EnumClass {
	A,
	B,
	C,
	D
};

struct TestObj {
	CR_DECLARE(TestObj);

	TestObj() {
		bvar = false;
		intvar = 0;
		fvar = 0.f;
		enumVar = A;
		for(int a=0;a<5;a++) sarray[a] = 0;
		children[0] = children[1] = 0;
		embeddedPtr = &embedded;
	}
	virtual ~TestObj() {
		if (children[0]) delete children[0];
	}

	bool bvar;
	int intvar;
	float fvar;
	EnumClass enumVar;
	std::string str;
	int sarray[5];
	std::vector<int> darray;

	EmbeddedObj* embeddedPtr;
	TestObj* children[2];
	EmbeddedObj embedded;

	//this test fails atm
	//std::vector<EmbeddedObj>  embeddeds;
	//std::vector<EmbeddedObj*> embeddedPtrs;
};

CR_BIND(TestObj, );
CR_REG_METADATA(TestObj, (
	CR_MEMBER(bvar),
	CR_MEMBER(intvar),
	CR_MEMBER(fvar),
	CR_MEMBER(enumVar),
	CR_MEMBER(str),
	CR_MEMBER(sarray),
	CR_MEMBER(darray),
	CR_MEMBER(children),
	CR_MEMBER(embedded),
	CR_MEMBER(embeddedPtr)//,
	//CR_MEMBER(embeddeds),
	//CR_MEMBER(embeddedPtrs)
));


static void savetest(std::ostream* os)
{
	// root obj
	TestObj* o = new TestObj;
	o->darray.push_back(3);
	o->bvar = true;
	o->intvar = 1;
	o->fvar = 666.666f;
	o->enumVar = EnumClass::C;
	o->str = "Hi!";
	for (int a=0;a<5;a++) o->sarray[a]=a+10;
	//o->embeddeds.resize(10);

	// secondary obj
	TestObj* c = new TestObj;
	c->intvar = 144;
	//c->embeddeds.resize(5);

	// link o & c
	o->children[0] = c;
	o->children[1] = c;
	//for (int a=0;a<o->embeddeds.size();a++) c->embeddedPtrs.push_back(&o->embeddeds[a]);
	//for (int a=0;a<c->embeddeds.size();a++) o->embeddedPtrs.push_back(&c->embeddeds[a]);

	// save
	creg::COutputStreamSerializer ss;
	ss.SavePackage(os, o, o->GetClass());

	delete(o);
}


static void* loadtest(std::istream* is)
{
	void* root;
	creg::Class* rootCls;

	// load
	creg::CInputStreamSerializer ss;
	ss.LoadPackage(is, root, rootCls);

	return root;
}


static bool test_creg_members(TestObj* obj)
{
	if (obj->darray.size() != 1 || obj->darray[0] != 3) return false;
	if (!obj->bvar) return false;
	if (obj->intvar != 1) return false;
	if (obj->fvar != 666.666f) return false;
	if (obj->enumVar != EnumClass::C) return false;
	if (obj->str != "Hi!") return false;

	for (int a=0; a<5; a++) if (obj->sarray[a] != a+10) return false;
	return true;
}


static bool test_creg_pointers(TestObj* obj)
{
	if (obj->children[0] != obj->children[1]) return false;
	if (obj->children[0]->intvar != 144) return false;
	if (obj->embeddedPtr != &obj->embedded) return false;

	/*TestObj* c = obj->children[0];
	if (obj->embeddeds.size() != 10) return false;
	if (  c->embeddeds.size() != 5)  return false;
	for (int a=0; a < obj->embeddeds.size(); a++) if (  c->embeddedPtrs[a] != &obj->embeddeds[a]) return false;
	for (int a=0; a <   c->embeddeds.size(); a++) if (obj->embeddedPtrs[a] !=   &c->embeddeds[a]) return false;*/

	return true;
}



BOOST_AUTO_TEST_CASE( BOOST_TEST_MODULE )
{
	// save state
	std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
	savetest(&ss);

	// load state
	TestObj* root = (TestObj*)loadtest(&ss);

	// test it
	BOOST_CHECK_MESSAGE(dynamic_cast<TestObj*>(root), "test root obj");
	BOOST_CHECK_MESSAGE(test_creg_members(root),      "test class members");
	BOOST_CHECK_MESSAGE(test_creg_pointers(root),     "test class pointers");

	delete root;
}
