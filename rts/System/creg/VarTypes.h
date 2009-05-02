/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen

Implementations of IType for specific types
*/

#ifndef CR_VARIABLE_TYPES_H
#define CR_VARIABLE_TYPES_H

#include "creg_cond.h"

namespace creg
{
	class BasicType : public IType
	{
	public:
		BasicType(BasicTypeID ID) : id(ID) {}
		~BasicType() {}

		void Serialize (ISerializer *s, void *instance);
		std::string GetName();

		BasicTypeID id;
	};

	class ObjectInstanceType : public IType
	{
	public:
		ObjectInstanceType(Class* objc) : objectClass(objc) {}
		~ObjectInstanceType() {}
		void Serialize (ISerializer *s, void *instance);
		std::string GetName();

		Class* objectClass;
	};

	class StringType : public DynamicArrayType<std::string>
	{
	public:
		StringType(boost::shared_ptr<IType> charType);
		std::string GetName();
	};

};

#endif

