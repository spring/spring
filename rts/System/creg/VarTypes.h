/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 

Implementations of IType for specific types
*/

#ifndef CR_VARIABLE_TYPES_H
#define CR_VARIABLE_TYPES_H

#include "creg.h"

namespace creg
{
	class BasicType : public IType
	{
	public:
		BasicType(BasicTypeID ID) : id(ID) {}
		~BasicType() {}

		void Serialize (ISerializer *s, void *instance);

		BasicTypeID id;
	};

	class StringType : public IType
	{
	public:
		StringType() {}
		~StringType() {}
		void Serialize (ISerializer *s, void *instance);
	};


	class ObjectInstanceType : public IType
	{
	public:
		ObjectInstanceType(Class* objc) : objectClass(objc) {}
		~ObjectInstanceType() {}
		void Serialize (ISerializer *s, void *instance);

		Class* objectClass;
	};

};

#endif

