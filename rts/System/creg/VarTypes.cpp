/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 
*/
#include "StdAfx.h"
#include "VarTypes.h"
#include <assert.h>

using namespace std;
using namespace creg;

// serialization code

// type instance allocators

void BasicType::Serialize (ISerializer *s, void *inst)
{
	switch (id) {
	case crInt:
	case crUInt:
		s->Serialize (inst, 4);
		break;
	case crShort:
	case crUShort:
		s->Serialize (inst, 2);
		break;
	case crChar:
	case crUChar:
		s->Serialize (inst, 1);
		break;
	case crFloat:
		s->Serialize (inst, 4);
		break;
	case crDouble:
		s->Serialize (inst, 8);
		break;
	case crBool:{
		// I'm not sure if bool is the same size on all compilers.. so it's stored as a byte
		if (s->IsWriting ())  {
			char v = *(bool*)inst ? 1 : 0;
			s->Serialize (&v,1);
		} else {
			char v;
			s->Serialize (&v,1);
			*(bool*)inst=!!v;
		}
		break;}
	}
}

std::string BasicType::GetName()
{
	switch(id) {
		case crInt: return "int";
		case crUInt: return "uint";
		case crShort: return "short";
		case crUShort: return "ushort";
		case crChar:  return "char";
		case crUChar: return "uchar";
		case crFloat: return "float";
		case crDouble: return "double";
		case crBool: return "bool";
	};
	return std::string();
}

IType* IType::CreateBasicType (BasicTypeID t)
{
	return SAFE_NEW BasicType (t);
}

std::string StringType::GetName()
{
	return "string";
}

IType* IType::CreateStringType ()
{
	DeduceType<char> charType;
	return SAFE_NEW DynamicArrayType<string> (charType.Get());
}

void ObjectInstanceType::Serialize (ISerializer *s, void *inst)
{
	s->SerializeObjectInstance (inst, objectClass);
}

std::string ObjectInstanceType::GetName()
{
	return objectClass->name;
}

IType* IType::CreateObjInstanceType (Class *objectType)
{
	return SAFE_NEW ObjectInstanceType (objectType);
}

IType* IType::CreateEnumeratedType (size_t size)
{
	switch (size) {
		case 1: return SAFE_NEW BasicType (crUChar);
		case 2: return SAFE_NEW BasicType (crUShort);
		case 4: return SAFE_NEW BasicType (crUInt);
	}
	return 0;
}

string StaticArrayBaseType::GetName()
{ 
	char sstr[16];
	SNPRINTF(sstr,16,"%d", size);
	return elemType->GetName() + "[" + std::string(sstr) + "]";
}
