/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Type matching using class templates (only class template support partial specialization)
 */

#ifndef _TYPE_DEDUCTION_H
#define _TYPE_DEDUCTION_H

#include <memory>
#include "creg_cond.h"

namespace creg {

// Default
// If none specialization was found assume it's a class.
template<typename T, typename Enable = void>
struct DeduceType {
	static_assert(std::is_same<typename std::remove_const<T>::type, typename std::remove_const<typename T::MyType>::type>::value, "class isn't creged");
	static std::unique_ptr<IType> Get() { return IType::CreateObjInstanceType(T::StaticClass(), sizeof(T)); }
};


// Class case
// Covered by default case above
//WARNING: Defining this one would break any class-specialization as for std::vector & std::string below)
/*template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_class<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return std::unique_ptr<IType>(IType::CreateObjInstanceType(T::StaticClass(), sizeof(T))); }
};*/

// Enum
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_enum<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return IType::CreateBasicType(crInt, sizeof(T)); }
};

// Integer+Boolean (of any size)
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_integral<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return IType::CreateBasicType(crInt, sizeof(T)); }
};

// Floating-Point (of any size)
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return IType::CreateBasicType(crFloat, sizeof(T)); }
};

// Synced Integer + Float
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
template<typename T>
struct DeduceType<SyncedPrimitive<T>, typename std::enable_if<std::is_integral<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return IType::CreateBasicType(crInt /*crSyncedInt*/, sizeof(T)); }
};

template<typename T>
struct DeduceType<SyncedPrimitive<T>, typename std::enable_if<std::is_floating_point<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return IType::CreateBasicType(crFloat /*crSyncedFloat*/, sizeof(T)); }
};
#endif

// helper
template<typename T>
class ObjectPointerType : public ObjectPointerBaseType
{
	static_assert(std::is_same<typename std::remove_const<T>::type, typename std::remove_const<typename T::MyType>::type>::value, "class isn't creged");
public:
	ObjectPointerType() : ObjectPointerBaseType(T::StaticClass(), sizeof(T*)) { }
	void Serialize(ISerializer *s, void *instance) {
		void **ptr = (void**)instance;
		if (s->IsWriting()) {
			s->SerializeObjectPtr(ptr, (*ptr != nullptr) ? ((T*)*ptr)->GetClass() : 0);
		} else {
			s->SerializeObjectPtr(ptr, objClass);
		}
	}
};

// Pointer type
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_pointer<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return std::unique_ptr<IType>(new ObjectPointerType<typename std::remove_pointer<T>::type>()); }
};

// Reference type, handled as a pointer
template<typename T>
struct DeduceType<T, typename std::enable_if<std::is_reference<T>::value>::type> {
	static std::unique_ptr<IType> Get() { return std::unique_ptr<IType>(new ObjectPointerType<typename std::remove_reference<T>::type>()); }
};



// C-style static array type
template<typename T, int N>
class StaticArrayType : public StaticArrayBaseType
{
public:
	typedef T ArrayType[N];
	StaticArrayType() : StaticArrayBaseType(DeduceType<T>::Get(), N * sizeof(T)) { }

	void Serialize(ISerializer* s, void* instance)
	{
		T* array = (T*) instance;

		for (int a = 0; a < N; a++) {
			DeduceType<T>::Get()->Serialize(s, &array[a]);
		}
	}
};

template<typename T, size_t ArraySize>
struct DeduceType<T[ArraySize]> {
	static std::unique_ptr<IType> Get() {
		return std::unique_ptr<IType>(new StaticArrayType<T, ArraySize>());
	}
};



// STL static array type
template<typename ArrayT>
class stlStaticArrayType : public StaticArrayBaseType
{
public:
	typedef typename ArrayT::value_type ElemT;

	stlStaticArrayType() : StaticArrayBaseType(DeduceType<ElemT>::Get(), sizeof(ArrayT)) {}

	void Serialize(ISerializer* s, void* instance)
	{
		ArrayT& array = *(ArrayT*) instance;

		for (size_t a = 0; a < array.size(); a++) {
			DeduceType<ElemT>::Get()->Serialize(s, &array[a]);
		}
	}
};

template<typename ElemT, size_t ArraySize>
struct DeduceType<std::array<ElemT, ArraySize>> {
	static std::unique_ptr<IType> Get() {
		return std::unique_ptr<IType>(new stlStaticArrayType< std::array<ElemT, ArraySize> >());
	}
};



template<typename VectorT>
class DynamicArrayType : public DynamicArrayBaseType
{
public:
	typedef typename VectorT::value_type ElemT;

	DynamicArrayType() : DynamicArrayBaseType(DeduceType<ElemT>::Get(), sizeof(VectorT)) {}
	~DynamicArrayType() {}

	void Serialize(ISerializer* s, void* inst) {
		VectorT& ct = *(VectorT*) inst;

		if (s->IsWriting()) {
			int size = (int) ct.size();
			s->SerializeInt(&size, sizeof(int));

			for (int a = 0; a < size; a++) {
				DeduceType<ElemT>::Get()->Serialize(s, &ct[a]);
			}
		} else {
			int size;
			s->SerializeInt(&size, sizeof(int));

			ct.clear();
			ct.resize(size);

			for (int a = 0; a < size; a++) {
				DeduceType<ElemT>::Get()->Serialize(s, &ct[a]);
			}
		}
	}
};


// Vector type (vector<ElemT>)
template<typename ElemT>
struct DeduceType<std::vector<ElemT>> {
	static std::unique_ptr<IType> Get() {
		return std::unique_ptr<IType>(new DynamicArrayType< std::vector<ElemT> >());
	}
};


template<typename T>
class BitArrayType : public DynamicArrayBaseType
{
public:
	typedef typename T::value_type ElemT;

	BitArrayType() : DynamicArrayBaseType(DeduceType<ElemT>::Get(), sizeof(T)) { }
	~BitArrayType() { }

	void Serialize(ISerializer* s, void* inst) {
		T* ct = (T*)inst;
		if (s->IsWriting()) {
			int size = (int)ct->size();
			s->SerializeInt(&size, sizeof(int));
			for (int a = 0; a < size; a++) {
				bool b = (*ct)[a];
				DeduceType<ElemT>::Get()->Serialize(s, &b);
			}
		} else {
			int size;
			s->SerializeInt(&size, sizeof(int));
			ct->resize(size);
			for (int a = 0; a < size; a++) {
				bool b;
				DeduceType<ElemT>::Get()->Serialize(s, &b);
				(*ct)[a] = b;
			}
		}
	}
};


// std::vector<bool> is not a std::vector but a BitArray instead!
template<>
struct DeduceType<std::vector<bool>> {
	static std::unique_ptr<IType> Get() {
		return std::unique_ptr<IType>(new BitArrayType<std::vector<bool> >());
	}
};

// String type
template<>
struct DeduceType<std::string> {
	static std::unique_ptr<IType> Get() { return IType::CreateStringType(); }
};






// GetType allows to use parameter type deduction to get the template argument for DeduceType
template<typename T>
std::unique_ptr<IType> GetType(T& var) {
	return DeduceType<T>::Get();
}
}

#endif // _TYPE_DEDUCTION_H

