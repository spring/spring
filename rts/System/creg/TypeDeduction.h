/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 

Type matching using class templates (only class template support partial specialization)
*/

// Undefined types return 0
template<typename T>
struct DeduceType {
	IType* Get () { return 0; }
};

template<typename T>
struct IsBasicType {
	enum {Yes=0, No=1};
};

// Support for a number of fundamental types
#define SUPPORT_FTYPE(T, typeID)			\
	template <>	 struct DeduceType <T> {		\
		IType* Get () { return IType::CreateBasicType (typeID); }	\
	};																\
	template <> struct IsBasicType <T> {							\
		enum {Yes=1, No=0 };										\
	};

	SUPPORT_FTYPE(int, crInt)
	SUPPORT_FTYPE(unsigned int, crUInt)
	SUPPORT_FTYPE(short, crShort)
	SUPPORT_FTYPE(unsigned short, crUShort)
	SUPPORT_FTYPE(char, crChar)
	SUPPORT_FTYPE(unsigned char, crUChar)
	SUPPORT_FTYPE(long, crInt) // Long is assumed to be an int (4 bytes)
	SUPPORT_FTYPE(unsigned long, crUInt)
	SUPPORT_FTYPE(float, crFloat)
	SUPPORT_FTYPE(double, crDouble)
	SUPPORT_FTYPE(bool, crBool)
	SUPPORT_FTYPE(float3, crFloat3)
#undef SUPPORT_FTYPE

// Pointer type (assumed to be a pointer to an Object)
template<typename T>
struct DeduceType <T *> {
	IType* Get () { return IType::CreatePointerToObjType (T::StaticClass()); }
};

// Static array type
template<typename T, size_t ArraySize>
struct DeduceType <T[ArraySize]> {
	IType* Get () { 
		DeduceType<T> subtype;
		return IType::CreateStaticArrayType (subtype.Get(), ArraySize);
	}
};

// Vector type (vector<T>)
template<typename T>
struct DeduceType < std::vector <T> > {
	IType* Get () { 
		DeduceType<T> elemtype;
		return new UnorderedContainer < std::vector<T> > (elemtype.Get());
	}
};
// Deque type (uses vector implementation)
template<typename T>
struct DeduceType < std::deque <T> > {
	IType* Get () { 
		DeduceType<T> elemtype;
		return new UnorderedContainer < std::deque<T> > (elemtype.Get());
	}
};
// List type
template<typename T>
struct DeduceType < std::list <T> > {
	IType* Get () { 
		DeduceType<T> elemtype;
		return new UnorderedContainer < std::list<T> > (elemtype.Get());
	}
};
// Set type
template<typename T>
struct DeduceType < std::set<T> > {
	IType* Get () {
		DeduceType<T> elemtype;
		return new OrderedContainer < std::set <T> > (elemtype.Get());
	}
};
// Multiset
template<typename T>
struct DeduceType < std::multiset<T> > {
	IType* Get () {
		DeduceType<T> elemtype;
		return new OrderedContainer < std::multiset<T> > (elemtype.Get());
	}
};

// String type
template<> struct DeduceType < std::string > {
	IType* Get () { return IType::CreateStringType (); }
};

// GetType allows to use parameter type deduction to get the template argument for DeduceType
template<typename T>
IType* GetType (T& var) {
	DeduceType<T> deduce;
	return deduce.Get();
}

