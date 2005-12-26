/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 
*/
#include "StdAfx.h"
#include "ClassReg.h"

using namespace creg;
using namespace std;

// -------------------------------------------------------------------
// Class Binder
// -------------------------------------------------------------------

ClassBinder* ClassBinder::binderList = 0;
map<string, Class*> ClassBinder::nameToClass;

ClassBinder::ClassBinder (const char *className, ClassBinder* baseClsBinder, IMemberRegistrator** mreg, void*(*instCreateProc)())
{
	class_ = 0;
	name = className;
	memberRegistrator = mreg;
	createProc = instCreateProc;
	base = baseClsBinder;

	// link to the list of class binders
	nextBinder = binderList;
	binderList = this;
}

void ClassBinder::InitializeClasses ()
{
	// Create Class instances
	for (ClassBinder *c = binderList; c; c = c->nextBinder)
		c->class_ = new Class;

	// Initialize class instances
	for (ClassBinder *c = binderList; c; c = c->nextBinder) {
		Class *cls = c->class_;

		cls->binder = c;
		cls->name = c->name;
		cls->base = c->base ? c->base->class_ : 0;
		nameToClass [cls->name] = cls;

		// Register members
		if (*c->memberRegistrator)
			(*c->memberRegistrator)->RegisterMembers (cls);
	}
}

Class* ClassBinder::GetClass (const string& name)
{
	map<string, Class*>::iterator c = nameToClass.find (name);
	if (c == nameToClass.end()) 
		return 0;
	return c->second;
}

// ------------------------------------------------------------------
// creg::Object: Base Object
// ------------------------------------------------------------------

CR_BIND(Object);


// ------------------------------------------------------------------
// creg::Class: Class description
// ------------------------------------------------------------------

Class::Class () : 
	base (0),
	binder (0)
{}

Class::~Class ()
{
	for (unsigned int a=0;a<members.size();a++)
		delete members[a];
	members.clear();
}

bool Class::IsSubclassOf (Class *other)
{
	for (Class *c = this; c; c = c->base)
		if (c == other) return true;

	return false;
}

void Class::AddMember (const char *name, IType* type, unsigned int offset)
{
	Member *member = new Member;

	member->name = name;
	member->offset = offset;
	member->type = type;

	members.push_back (member);
}


