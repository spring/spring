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

// Default
// If none specialization was found assume it's a class.
template<typename T, typename Enable = void>
struct DeduceType {
	static_assert(std::is_same<typename std::remove_const<T>::type, typename std::remove_const<typename T::MyType>::type>::value, "class isn't creged");
	static boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(IType::CreateObjInstanceType(T::StaticClass())); }
};


// Class case
// Covered by default case above
//WARNING: Defining this one would break any class-specialization as for std::vector & std::string below)
/*template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_class<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(IType::CreateObjInstanceType(T::StaticClass())); }
};*/

// Enum
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_enum<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return IType::CreateBasicType(crInt, sizeof(T)); }
};

// Integer+Boolean (of any size)
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_integral<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return IType::CreateBasicType(crInt, sizeof(T)); }
};

// Floating-Point (of any size)
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return IType::CreateBasicType(crFloat, sizeof(T)); }
};

// Synced Integer + Float
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
template<typename T>
struct DeduceType<SyncedPrimitive<T>, typename std::enable_if<std::is_integral<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return IType::CreateBasicType(crInt /*crSyncedInt*/, sizeof(T)); }
};

template<typename T>
struct DeduceType<SyncedPrimitive<T>, typename std::enable_if<std::is_floating_point<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return IType::CreateBasicType(crFloat /*crSyncedFloat*/, sizeof(T)); }
};
#endif

// helper
template<typename T>
class ObjectPointerType : public IType
{
	static_assert(std::is_same<typename std::remove_const<T>::type, typename std::remove_const<typename T::MyType>::type>::value, "class isn't creged");
public:
	ObjectPointerType() { objectClass = T::StaticClass(); }
	void Serialize(ISerializer *s, void *instance) {
		void **ptr = (void**)instance;
		if (s->IsWriting())
			s->SerializeObjectPtr(ptr, *ptr ? ((T*)*ptr)->GetClass() : 0);
		else s->SerializeObjectPtr(ptr, objectClass);
	}
	std::string GetName() const { return objectClass->name + "*"; }
	size_t GetSize() const { return sizeof(T*); }
	Class* objectClass;
};

// Pointer type
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_pointer<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(new ObjectPointerType<typename std::remove_pointer<T>::type>()); }
};

// Reference type, handled as a pointer
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_reference<T>::value>::type> {
	static boost::shared_ptr<IType> Get() { return boost::shared_ptr<IType>(new ObjectPointerType<typename std::remove_reference<T>::type>()); }
};

// Static array type
template<typename T, size_t ArraySize>
struct DeduceType<T[ArraySize]> {
	static boost::shared_ptr<IType> Get() {
		return boost::shared_ptr<IType>(new StaticArrayType<T, ArraySize>(DeduceType<T>::Get()));
	}
};

// Vector type (vector<T>)
template<typename T>
struct DeduceType<std::vector<T>> {
	static boost::shared_ptr<IType> Get() {
		return boost::shared_ptr<IType>(new DynamicArrayType<std::vector<T> >(DeduceType<T>::Get()));
	}
};

// std::vector<bool> is not a std::vector but a BitArray instead!
template<>
struct DeduceType<std::vector<bool>> {
	static boost::shared_ptr<IType> Get() {
		return boost::shared_ptr<IType>(new BitArrayType<std::vector<bool> >(DeduceType<bool>::Get()));
	}
};

// String type
template<>
struct DeduceType<std::string> {
	static boost::shared_ptr<IType> Get() { return IType::CreateStringType(); }
};






// GetType allows to use parameter type deduction to get the template argument for DeduceType
template<typename T>
boost::shared_ptr<IType> GetType(T& var) {
	return DeduceType<T>::Get();
}
}

#endif // _TYPE_DEDUCTION_H

