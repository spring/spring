/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Classes for serialization of registrated class instances
 */

#include <map>
#include <vector>
#include <string>
#include <string.h>


// creg has to be made aware of mmgr explicitly
#define	operator_new		::operator new
#define	operator_delete		::operator delete

#include "System/Util.h"

#include "creg_cond.h"

using namespace creg;

static std::map<std::string, Class*> mapNameToClass;
static int currentMemberFlags = 0; // used when registering class members

// -------------------------------------------------------------------
// Class Binder
// -------------------------------------------------------------------

std::vector<ClassBinder*> System::binders;
std::vector<Class*> System::classes;


ClassBinder::ClassBinder(const char* className, unsigned int cf,
		ClassBinder* baseClsBinder, IMemberRegistrator** mreg, int instanceSize, int instanceAlignment, bool hasVTable, bool isCregStruct,
		void (*constructorProc)(void* inst), void (*destructorProc)(void* inst))
	: class_()
	, base(baseClsBinder)
	, flags((ClassFlags)cf)
	, memberRegistrator(mreg)
	, name(className)
	, size(instanceSize)
	, alignment(instanceAlignment)
	, hasVTable(hasVTable)
	, isCregStruct(isCregStruct)
	, constructor(constructorProc)
	, destructor(destructorProc)
{
	// link to the list of class binders
	System::AddClassBinder(this);
}

void System::InitializeClasses()
{
	if (!classes.empty())
		return;

	// Initialize class instances
	for (ClassBinder* c: binders) {
		Class& cls = c->class_;

		cls.binder = c;
		cls.name = c->name;
		cls.size = c->size;
		cls.alignment = c->alignment;
		cls.base = c->base ? &c->base->class_ : NULL;
		mapNameToClass[cls.name] = &cls;

		if (cls.base) {
			cls.base->derivedClasses.push_back(&cls);
		}

		currentMemberFlags = 0;
		// Register members
		if (*c->memberRegistrator) {
			(*c->memberRegistrator)->RegisterMembers(&cls);
		}

		classes.push_back(&cls);
	}
}

Class* System::GetClass(const std::string& name)
{
	std::map<std::string, Class*>::const_iterator c = mapNameToClass.find(name);
	if (c == mapNameToClass.end()) {
		return NULL;
	}
	return c->second;
}

void System::AddClassBinder(ClassBinder* cb)
{
	binders.push_back(cb);
}

// ------------------------------------------------------------------
// creg::Class: Class description
// ------------------------------------------------------------------

Class::Class()
: binder(nullptr)
, size(0)
, alignment(0)
, base(nullptr)
{}

Class::~Class()
{
	for (Member* m: members) {
		delete m;
	}
	members.clear();
}

bool Class::IsSubclassOf(Class* other) const
{
	for (const Class* c = this; c; c = c->base) {
		if (c == other) {
			return true;
		}
	}

	return false;
}

std::vector<Class*> Class::GetImplementations()
{
	std::vector<Class*> classes;

	for (unsigned int a = 0; a < derivedClasses.size(); a++) {
		Class* dc = derivedClasses[a];
		if (!dc->IsAbstract()) {
			classes.push_back(dc);
		}

		const std::vector<Class*>& impl = dc->GetImplementations();
		classes.insert(classes.end(), impl.begin(), impl.end());
	}

	return classes;
}

void Class::BeginFlag(ClassMemberFlag flag)
{
	currentMemberFlags |= (int)flag;
}

void Class::EndFlag(ClassMemberFlag flag)
{
	currentMemberFlags &= ~(int)flag;
}

void Class::SetFlag(ClassFlags flag)
{
	binder->flags = (ClassFlags) (binder->flags | flag);
}

bool Class::AddMember(const char* name, IType* type, unsigned int offset, int alignment)
{
	if (FindMember(name))
		return false;

	Member* member = new Member;

	member->name = name;
	member->offset = offset;
	member->type = boost::shared_ptr<IType>(type);
	member->alignment = alignment;
	member->flags = currentMemberFlags;

	members.push_back(member);
	return true;
}

bool Class::AddMember(const char* name, boost::shared_ptr<IType> type, unsigned int offset, int alignment)
{
	if (FindMember(name))
		return false;

	Member* member = new Member;

	member->name = name;
	member->offset = offset;
	member->type = type;
	member->alignment = alignment;
	member->flags = currentMemberFlags;

	members.push_back(member);
	return true;
}

Class::Member* Class::FindMember(const char* name)
{
	for (Class* c = this; c; c = c->base) {
		for (uint a = 0; a < c->members.size(); a++) {
			Member* member = c->members[a];
			if (!STRCASECMP(member->name, name))
				return member;
		}
	}
	return NULL;
}

void Class::SetMemberFlag(const char* name, ClassMemberFlag f)
{
	for (uint a = 0; a < members.size(); a++) {
		if (!strcmp(members[a]->name, name)) {
			members[a]->flags |= (int)f;
			break;
		}
	}
}

void* Class::CreateInstance()
{
	void* inst = operator_new(binder->size);

	if (binder->constructor) {
		binder->constructor(inst);
	}
	return inst;
}

void Class::DeleteInstance(void* inst)
{
	if (binder->destructor) {
		binder->destructor(inst);
	}

	operator_delete(inst);
}

static void StringHash(const std::string& str, unsigned int& hash)
{
	// FIXME use HsiehHash.h?
	unsigned int a = 63689;
	for (std::string::const_iterator si = str.begin(); si != str.end(); ++si) {
		hash = hash * a + (*si);
		a *= 378551;
	}
}

void Class::CalculateChecksum(unsigned int& checksum)
{
	for (size_t a = 0; a < members.size(); a++) {
		Member* m = members[a];
		checksum += m->flags;
		StringHash(m->name, checksum);
		StringHash(m->type->GetName(), checksum);
		checksum += m->type->GetSize();
	}
	if (base) {
		base->CalculateChecksum(checksum);
	}
}


IType::~IType() {
}

IMemberRegistrator::~IMemberRegistrator() {
}

