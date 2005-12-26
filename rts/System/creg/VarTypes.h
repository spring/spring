/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 

Implementations of IType for specific types
*/

#ifndef CR_VARIABLE_TYPES_H
#define CR_VARIABLE_TYPES_H

#include "ClassReg.h"

namespace creg
{
	class BasicType : public IType
	{
	public:
		BasicType(BasicTypeID ID) : id(ID) {}
		~BasicType() {}

		BasicTypeID id;
	};

	class StringType : public IType
	{
	public:
		StringType() {}
		~StringType() {}
	};

	class ObjectPointerType : public IType
	{
	public:
		ObjectPointerType(Class* objc) : objectClass(objc) {}
		~ObjectPointerType() {}

		Class* objectClass;
	};

	class StaticArrayType : public IType
	{
	public:
		StaticArrayType(IType *et, size_t s) : elementType(et), size(s) {}
		~StaticArrayType();

		IType* elementType;
		size_t size;
	};
};

#endif

