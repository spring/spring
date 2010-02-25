/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Classes for serialization of registrated class instances
 */

#include "StdAfx.h"
#include <assert.h>
#include "mmgr.h"

#include "VarTypes.h"
#include "Util.h"

using namespace std;
using namespace creg;

// serialization code

// type instance allocators

void BasicType::Serialize (ISerializer *s, void *inst)
{
	switch (id) {
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	case crSyncedSint://FIXME
	case crSyncedUint:
#endif
	case crInt:
	case crUInt:
		s->SerializeInt (inst, 4);
		break;
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	case crSyncedSshort://FIXME
	case crSyncedUshort:
#endif
	case crShort:
	case crUShort:
		s->SerializeInt (inst, 2);
		break;
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	case crSyncedSchar://FIXME
	case crSyncedUchar:
#endif
	case crChar:
	case crUChar:
		s->Serialize (inst, 1);
		break;
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	case crSyncedFloat://FIXME
#endif
	case crFloat:
		s->Serialize (inst, 4);
		break;
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	case crSyncedDouble://FIXME
#endif
	case crDouble:
		s->Serialize (inst, 8);
		break;
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	case crSyncedBool://FIXME
#endif
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
		case crFloat: return "float";
		case crDouble: return "double";
		case crBool: return "bool";
	};
	return std::string();
}

boost::shared_ptr<IType> IType::CreateBasicType (BasicTypeID t)
{
	return boost::shared_ptr<IType>(new BasicType (t));
}

std::string StringType::GetName()
{
	return "string";
}

StringType::StringType(boost::shared_ptr<IType> charType) : DynamicArrayType<string> (charType) {}

boost::shared_ptr<IType> IType::CreateStringType ()
{
	DeduceType<char> charType;
	return boost::shared_ptr<IType>(new StringType(charType.Get()));
}

void ObjectInstanceType::Serialize (ISerializer *s, void *inst)
{
	s->SerializeObjectInstance (inst, objectClass);
}

std::string ObjectInstanceType::GetName()
{
	return objectClass->name;
}

boost::shared_ptr<IType> IType::CreateObjInstanceType (Class *objectType)
{
	return boost::shared_ptr<IType>(new ObjectInstanceType (objectType));
}

boost::shared_ptr<IType> IType::CreateEnumeratedType (size_t size)
{
	switch (size) {
		case 1: return boost::shared_ptr<IType>(new BasicType (crUChar));
		case 2: return boost::shared_ptr<IType>(new BasicType (crUShort));
		case 4: return boost::shared_ptr<IType>(new BasicType (crUInt));
		default: assert(false); break;
	}
	return boost::shared_ptr<IType>(new EmptyType(0));
}

string StaticArrayBaseType::GetName()
{
	char sstr[16];
	SNPRINTF(sstr,16,"%d", size);
	return elemType->GetName() + "[" + std::string(sstr) + "]";
}
