/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Classes for serialization of registrated class instances
 */

#include <unordered_map>
#include <vector>
#include <string>
#include <string.h>

#include "creg_cond.h"
#include "System/Util.h"
#include "System/Sync/HsiehHash.h"


using namespace creg;

// -------------------------------------------------------------------
// some local statics, needed cause we work with global static vars
// -------------------------------------------------------------------

static std::unordered_map<const Class*, std::vector<Class*>>& derivedClasses()
{
	// note: we cannot save this in `class Class`, cause those are created with
	//   global statics, and those have an arbitrary init order. And when a Class
	//   gets created, it doesn't mean its parent class already has. (Still its
	//   pointer is already valid!)
	//   So we cannot access other's Class' members that early in the loading
	//   stage, that's solved with these local static vars, which get created
	//   on access.
	static std::unordered_map<const Class*, std::vector<Class*>> m;
	return m;
}

static std::unordered_map<std::string, Class*>& mapNameToClass()
{
	static std::unordered_map<std::string, Class*> m;
	return m;
}

static std::vector<Class*>& classes()
{
	static std::vector<Class*> v;
	return v;
}

// -------------------------------------------------------------------
// Class Binder
// -------------------------------------------------------------------

ClassBinder::ClassBinder(const char* className, ClassFlags cf,
		ClassBinder* baseClsBinder, void (*memberRegistrator)(creg::Class*), int instanceSize, int instanceAlignment, bool hasVTable, bool isCregStruct,
		void (*constructorProc)(void* inst), void (*destructorProc)(void* inst))
	: class_(className)
	, base(baseClsBinder)
	, flags(cf)
	, name(className)
	, size(instanceSize)
	, alignment(instanceAlignment)
	, hasVTable(hasVTable)
	, isCregStruct(isCregStruct)
	, constructor(constructorProc)
	, destructor(destructorProc)
{
	class_.binder = this;
	class_.size = instanceSize;
	class_.alignment = instanceAlignment;
	System::AddClass(&class_);

	if (base) {
		derivedClasses()[&base->class_].push_back(&class_);
	}

	// Register members
	assert(memberRegistrator);
	memberRegistrator(&class_);
}


// -------------------------------------------------------------------
// System
// -------------------------------------------------------------------

const std::vector<Class*>& System::GetClasses()
{
	return classes();
}

Class* System::GetClass(const std::string& name)
{
	const auto it = mapNameToClass().find(name);
	if (it == mapNameToClass().end()) {
		return NULL;
	}
	return it->second;
}

void System::AddClass(Class* c)
{
	classes().push_back(c);
	mapNameToClass()[c->name] = c;
}


// ------------------------------------------------------------------
// creg::Class: Class description
// ------------------------------------------------------------------

Class::Class(const char* _name)
: binder(nullptr)
, size(0)
, alignment(0)
, currentMemberFlags(0)
{
	name = _name;
}


bool Class::IsSubclassOf(Class* other) const
{
	for (const Class* c = this; c; c = c->base()) {
		if (c == other) {
			return true;
		}
	}

	return false;
}

std::vector<Class*> Class::GetImplementations()
{
	std::vector<Class*> classes;

	for (Class* dc: derivedClasses()[this]) {
		if (!dc->IsAbstract()) {
			classes.push_back(dc);
		}

		const std::vector<Class*>& impl = dc->GetImplementations();
		classes.insert(classes.end(), impl.begin(), impl.end());
	}

	return classes;
}


const std::vector<Class*>& Class::GetDerivedClasses() const
{
	return derivedClasses()[this];
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

void Class::AddMember(const char* name, boost::shared_ptr<IType> type, unsigned int offset, int alignment)
{
	assert(!FindMember(name, false));

	members.emplace_back();
	Member& m = members.back();

	m.name = name;
	m.offset = offset;
	m.type = type;
	m.alignment = alignment;
	m.flags = currentMemberFlags;
}

Class::Member* Class::FindMember(const char* name, const bool inherited)
{
	for (Class* c = this; c; c = c->base()) {
		for (Member& m: members) {
			if (!STRCASECMP(m.name, name))
				return &m;
		}
		if (!inherited)
			return nullptr;
	}
	return nullptr;
}

void Class::SetMemberFlag(const char* name, ClassMemberFlag f)
{
	for (Member& m: members) {
		if (!strcmp(m.name, name)) {
			m.flags |= (int)f;
			break;
		}
	}
}

void* Class::CreateInstance()
{
	void* inst = ::operator new(binder->size);

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

	::operator delete(inst);
}

void Class::CalculateChecksum(unsigned int& checksum)
{
	for (Member& m: members) {
		checksum += m.flags;
		checksum = HsiehHash(m.name, strlen(m.name), checksum);
		checksum = HsiehHash(m.type->GetName().data(), m.type->GetName().size(), checksum);
		checksum += m.type->GetSize();
	}
	if (base()) {
		base()->CalculateChecksum(checksum);
	}
}

