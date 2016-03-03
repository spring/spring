/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Classes for serialization of registrated class instances
 */

#include "VarTypes.h"

#include "System/Util.h"

#include <assert.h>

using namespace creg;
using std::string;

// serialization code

// type instance allocators

void BasicType::Serialize(ISerializer* s, void* inst)
{
	s->SerializeInt(inst, GetSize());
}

std::string BasicType::GetName() const
{
	switch(id) {
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
		case crSyncedInt:   return "synced int";
		case crSyncedFloat: return "synced float";
#endif
		case crInt:   return "int";
		case crFloat: return "float";
	};
	return std::string();
}

size_t BasicType::GetSize() const
{
	return size;
}

boost::shared_ptr<IType> IType::CreateBasicType(BasicTypeID t, size_t size)
{
	return boost::shared_ptr<IType>(new BasicType(t, size));
}

std::string StringType::GetName() const
{
	return "string";
}

size_t StringType::GetSize() const
{
	return sizeof(std::string);
}

StringType::StringType(boost::shared_ptr<IType> charType) : DynamicArrayType<string>(charType) {}

boost::shared_ptr<IType> IType::CreateStringType()
{
	DeduceType<char> charType;
	return boost::shared_ptr<IType>(new StringType(charType.Get()));
}

void ObjectInstanceType::Serialize(ISerializer* s, void* inst)
{
	s->SerializeObjectInstance(inst, objectClass);
}

std::string ObjectInstanceType::GetName() const
{
	return objectClass->name;
}

size_t ObjectInstanceType::GetSize() const
{
	return objectClass->size;
}

boost::shared_ptr<IType> IType::CreateObjInstanceType(Class* objectType)
{
	return boost::shared_ptr<IType>(new ObjectInstanceType(objectType));
}

string StaticArrayBaseType::GetName() const
{
	char sstr[16];
	SNPRINTF(sstr, 16, "%d", size);
	return elemType->GetName() + "[" + std::string(sstr) + "]";
}

boost::shared_ptr<IType> IType::CreateIgnoredType(size_t size)
{
	return boost::shared_ptr<IType>(new IgnoredType(size));
}
