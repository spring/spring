/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 
*/
#include "StdAfx.h"
#include "VarTypes.h"

using namespace creg;

// type instance allocators

IType* IType::CreateBasicType (BasicTypeID t)
{
	return new BasicType (t);
}

IType* IType::CreateStringType ()
{
	return new StringType;
}

IType* IType::CreatePointerToObjType (Class *objectType)
{
	return new ObjectPointerType (objectType);
}

IType* IType::CreateStaticArrayType (IType *elemType, unsigned int size)
{
	return new StaticArrayType (elemType, size);
}


// container type

IContainerType::~IContainerType ()
{
	if (elemType)
		delete elemType;
}

// static array type

StaticArrayType::~StaticArrayType ()
{
	if (elementType)
		delete elementType;
}
