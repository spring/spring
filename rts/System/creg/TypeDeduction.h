/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Type matching using class templates (only class template support partial specialization)
 */

#ifndef _TYPE_DEDUCTION_H
#define _TYPE_DEDUCTION_H

#include <boost/shared_ptr.hpp>
#include "creg_cond.h"
#include <boost/cstdint.hpp>

namespace creg {

// Undefined types return 0
template<typename T>
struct DeduceType {
	boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(IType::CreateObjInstanceType(T::StaticClass())); }
};

template<typename T>
struct IsBasicType {
	enum {Yes=0, No=1};
};

// Support for a number of fundamental types
#define CREG_SUPPORT_BASIC_TYPE(T, typeID)			\
	template<>	 struct DeduceType<T> {		\
		boost::shared_ptr<IType> Get() { return IType::CreateBasicType(typeID); }	\
	};																\
	template<> struct IsBasicType<T> {							\
		enum {Yes=1, No=0 };										\
	};

CREG_SUPPORT_BASIC_TYPE(boost::int64_t, crInt64)
CREG_SUPPORT_BASIC_TYPE(int, crInt)
CREG_SUPPORT_BASIC_TYPE(unsigned int, crUInt)
CREG_SUPPORT_BASIC_TYPE(short, crShort)
CREG_SUPPORT_BASIC_TYPE(unsigned short, crUShort)
CREG_SUPPORT_BASIC_TYPE(char, crChar)
CREG_SUPPORT_BASIC_TYPE(unsigned char, crUChar)
CREG_SUPPORT_BASIC_TYPE(unsigned long, crUInt)
CREG_SUPPORT_BASIC_TYPE(float, crFloat)
CREG_SUPPORT_BASIC_TYPE(double, crDouble)
CREG_SUPPORT_BASIC_TYPE(bool, crBool)

#if defined(SYNCDEBUG) || defined(SYNCCHECK)
CREG_SUPPORT_BASIC_TYPE(SyncedSint,   crSyncedSint)
CREG_SUPPORT_BASIC_TYPE(SyncedUint,   crSyncedUint)
CREG_SUPPORT_BASIC_TYPE(SyncedSshort, crSyncedSshort)
CREG_SUPPORT_BASIC_TYPE(SyncedUshort, crSyncedUshort)
CREG_SUPPORT_BASIC_TYPE(SyncedSchar,  crSyncedSchar)
CREG_SUPPORT_BASIC_TYPE(SyncedUchar,  crSyncedUchar)
CREG_SUPPORT_BASIC_TYPE(SyncedFloat,  crSyncedFloat)
CREG_SUPPORT_BASIC_TYPE(SyncedDouble, crSyncedDouble)
CREG_SUPPORT_BASIC_TYPE(SyncedBool,   crSyncedBool)
#endif


template<typename T>
class ObjectPointerType : public IType
{
public:
	ObjectPointerType() { objectClass = T::StaticClass(); }
	void Serialize(ISerializer *s, void *instance) {
		void **ptr = (void**)instance;
		if (s->IsWriting())
			s->SerializeObjectPtr(ptr, *ptr ? ((T*)*ptr)->GetClass() : 0);
		else s->SerializeObjectPtr(ptr, objectClass);
	}
	std::string GetName() { return objectClass->name + "*"; }
	size_t GetSize() { return sizeof(T*); }
	Class* objectClass;
};

// Pointer type
template<typename T>
struct DeduceType<T*> {
	boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(new ObjectPointerType<T>()); }
};

// Reference type, handled as a pointer
template<typename T>
struct DeduceType<T&> {
	boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(new ObjectPointerType<T>()); }
};

// Static array type
template<typename T, size_t ArraySize>
struct DeduceType<T[ArraySize]> {
	boost::shared_ptr<IType> Get() {
		DeduceType<T> subtype;
		return boost::shared_ptr<IType>(new StaticArrayType<T, ArraySize>(subtype.Get()));
	}
};

// Vector type (vector<T>)
template<typename T>
struct DeduceType<std::vector<T> > {
	boost::shared_ptr<IType> Get() {
		DeduceType<T> elemtype;
		return boost::shared_ptr<IType>(new DynamicArrayType<std::vector<T> >(elemtype.Get()));
	}
};

// std::vector<bool> is not a std::vector but a BitArray instead!
template<>
struct DeduceType<std::vector<bool> > {
	boost::shared_ptr<IType> Get() {
		DeduceType<bool> elemtype;
		return boost::shared_ptr<IType>(new BitArrayType<std::vector<bool> >(elemtype.Get()));
	}
};

// String type
template<> struct DeduceType<std::string > {
	boost::shared_ptr<IType> Get() { return IType::CreateStringType(); }
};

// GetType allows to use parameter type deduction to get the template argument for DeduceType
template<typename T>
boost::shared_ptr<IType> GetType(T& var) {
	DeduceType<T> deduce;
	return deduce.Get();
}
};

#endif // _TYPE_DEDUCTION_H

