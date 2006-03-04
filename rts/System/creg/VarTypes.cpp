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

IType* IType::CreateBasicType (BasicTypeID t)
{
	return new BasicType (t);
}

IType* IType::CreateStringType ()
{
	DeduceType<char> charType;
	return new DynamicArrayType<string> (charType.Get());
}

void ObjectInstanceType::Serialize (ISerializer *s, void *inst)
{
	s->SerializeObjectInstance (inst, objectClass);
}

IType* IType::CreateObjInstanceType (Class *objectType)
{
	return new ObjectInstanceType (objectType);
}

IType* IType::CreateEnumeratedType (size_t size)
{
	switch (size) {
		case 1: return new BasicType (crUChar);
		case 2: return new BasicType (crUShort);
		case 4: return new BasicType (crUInt);
	}
	return 0;
}
