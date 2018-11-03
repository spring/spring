/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Classes for serialization of registrated class instances
 */

#include "VarTypes.h"

#include "System/StringUtil.h"

#include <cassert>

using namespace creg;

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

std::unique_ptr<IType> IType::CreateBasicType(BasicTypeID t, size_t size)
{
	return std::unique_ptr<IType>(new BasicType(t, size));
}

std::string ObjectPointerBaseType::GetName() const
{
	return std::string(objClass->name) + "*";
}

std::string StringType::GetName() const
{
	return "string";
}

std::unique_ptr<IType> IType::CreateStringType()
{
	return std::unique_ptr<IType>(new StringType());
}

void ObjectInstanceType::Serialize(ISerializer* s, void* inst)
{
	s->SerializeObjectInstance(inst, objectClass);
}

std::string ObjectInstanceType::GetName() const
{
	return objectClass->name;
}

std::unique_ptr<IType> IType::CreateObjInstanceType(Class* objectType, size_t size)
{
	return std::unique_ptr<IType>(new ObjectInstanceType(objectType, size));
}

std::string StaticArrayBaseType::GetName() const
{
	return elemType->GetName() + "[" + IntToString(size / elemType->GetSize()) + "]";
}

std::string DynamicArrayBaseType::GetName() const
{
	return elemType->GetName() + "[]";
}

std::unique_ptr<IType> IType::CreateIgnoredType(size_t size)
{
	return std::unique_ptr<IType>(new IgnoredType(size));
}
