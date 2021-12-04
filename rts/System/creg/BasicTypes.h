/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_BASIC_TYPES_H
#define CR_BASIC_TYPES_H

namespace creg {
	class ObjectPointerBaseType : public IType
	{
	public:
		Class* objClass;

		ObjectPointerBaseType(Class* cls, size_t size)
			: IType(size), objClass(cls) { }
		~ObjectPointerBaseType() { }

		std::string GetName() const override;
	};

	class StaticArrayBaseType : public IType
	{
	public:
		std::unique_ptr<IType> elemType;

		StaticArrayBaseType(std::unique_ptr<IType> et, size_t size)
			: IType(size), elemType(std::move(et)) { }
		~StaticArrayBaseType() { }

		std::string GetName() const override;
	};

	class DynamicArrayBaseType : public IType
	{
	public:
		std::unique_ptr<IType> elemType;

		DynamicArrayBaseType(std::unique_ptr<IType> et, size_t size)
			: IType(size), elemType(std::move(et)) { }
		~DynamicArrayBaseType() { }

		std::string GetName() const override;
	};

	class IgnoredType : public IType
	{
	public:
		IgnoredType(int size) : IType(size) { }
		~IgnoredType() {}

		void Serialize(ISerializer* s, void* instance) override
		{
			for (int a=0;a<size;a++) {
				char c=0;
				s->Serialize(&c,1);
			}
		}
		std::string GetName() const override
		{
			return "ignored";
		}
	};

	class BasicType : public IType
	{
	public:
		BasicType(BasicTypeID ID, size_t size) : IType(size), id(ID) {}
		~BasicType() {}

		void Serialize(ISerializer* s, void* instance) override;
		std::string GetName() const override;

		BasicTypeID id;
	};
}

#endif
