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

std::string BasicType::GetName()
{
	switch(id) {
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
		case crSyncedSint: return "int"; //FIXME
		case crSyncedUint: return "uint";
		case crSyncedSshort: return "short";
		case crSyncedUshort: return "ushort";
		case crSyncedSchar: return "char";
		case crSyncedUchar: return "uchar";
		case crSyncedFloat: return "float";
		case crSyncedDouble: return "double";
		case crSyncedBool: return "bool";
#endif
		case crInt: return "int";
		case crUInt: return "uint";
		case crShort: return "short";
		case crUShort: return "ushort";
		case crChar:  return "char";
		case crUChar: return "uchar";
		case crInt64: return "int64";
		case crUInt64: return "uint64";
		case crFloat: return "float";
		case crDouble: return "double";
		case crBool: return "bool";
	};
	return std::string();
}

size_t BasicType::GetSize()
{
	return size;
}

boost::shared_ptr<IType> IType::CreateBasicType(BasicTypeID t, size_t size)
{
	return boost::shared_ptr<IType>(new BasicType(t, size));
}

std::string StringType::GetName()
{
	return "string";
}

size_t StringType::GetSize()
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

std::string ObjectInstanceType::GetName()
{
	return objectClass->name;
}

size_t ObjectInstanceType::GetSize()
{
	return objectClass->size;
}

boost::shared_ptr<IType> IType::CreateObjInstanceType(Class* objectType)
{
	return boost::shared_ptr<IType>(new ObjectInstanceType(objectType));
}

string StaticArrayBaseType::GetName()
{
	char sstr[16];
	SNPRINTF(sstr, 16, "%d", size);
	return elemType->GetName() + "[" + std::string(sstr) + "]";
}
