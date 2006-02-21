/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 
*/
#include "StdAfx.h"
#include "creg.h"
#include <map>

using namespace creg;
using namespace std;

static map<string, Class*> mapNameToClass;

// -------------------------------------------------------------------
// Class Binder
// -------------------------------------------------------------------

ClassBinder* ClassBinder::binderList = 0;
vector<Class*> ClassBinder::classes;

ClassBinder::ClassBinder (const char *className, ClassFlags cf, ClassBinder* baseClsBinder, IMemberRegistrator** mreg, int instanceSize, void (*constructorProc)(void *Inst), void (*destructorProc)(void *Inst))
{
	class_ = 0;
	name = className;
	memberRegistrator = mreg;
	constructor = constructorProc;
	destructor = destructorProc;
	base = baseClsBinder;
	size = instanceSize;
	flags = cf;

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
		mapNameToClass [cls->name] = cls;

		// Register members
		if (*c->memberRegistrator)
			(*c->memberRegistrator)->RegisterMembers (cls);

		classes.push_back (cls);
	}
}

void ClassBinder::FreeClasses ()
{
	for (int a=0;a<classes.size();a++)
		delete classes[a];
	classes.clear();
}

Class* ClassBinder::GetClass (const string& name)
{
	map<string, Class*>::iterator c = mapNameToClass.find (name);
	if (c == mapNameToClass.end()) 
		return 0;
	return c->second;
}

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

void Class::SerializeInstance (ISerializer *s, void *ptr)
{
	if (base)
		base->SerializeInstance (s, ptr);

	for (int a=0;a<members.size();a++)
	{
		creg::Class::Member* m = members [a];
		void *memberAddr = ((char*)ptr) + m->offset;

		m->type->Serialize (s, memberAddr);
	}
}
