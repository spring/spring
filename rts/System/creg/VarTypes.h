/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Implementations of IType for specific types
 */

#ifndef CR_VARIABLE_TYPES_H
#define CR_VARIABLE_TYPES_H

#include "creg_cond.h"

namespace creg
{
	class ObjectInstanceType : public IType
	{
	public:
		ObjectInstanceType(Class* objc) : objectClass(objc) {}
		~ObjectInstanceType() {}
		void Serialize(ISerializer* s, void* instance);
		std::string GetName() const;
		size_t GetSize() const;

		Class* objectClass;
	};

	class StringType : public DynamicArrayType<std::string>
	{
	public:
		StringType(std::shared_ptr<IType> charType);
		std::string GetName() const;
		size_t GetSize() const;
	};

}

#endif

