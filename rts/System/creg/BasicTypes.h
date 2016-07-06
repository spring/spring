/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_BASIC_TYPES_H
#define CR_BASIC_TYPES_H

namespace creg {
	// vector,deque container
	template<typename T>
	class DynamicArrayType : public IType
	{
	public:
		typedef typename T::iterator iterator;
		typedef typename T::value_type ElemT;

		boost::shared_ptr<IType> elemType;

		DynamicArrayType(boost::shared_ptr<IType> et)
			: elemType(et) {}
		~DynamicArrayType() {}

		void Serialize(ISerializer* s, void* inst) {
			T& ct = *(T*)inst;
			if (s->IsWriting()) {
				int size = (int)ct.size();
				s->SerializeInt(&size, sizeof(int));
				for (int a = 0; a < size; a++) {
					elemType->Serialize(s, &ct[a]);
				}
			} else {
				ct.clear();
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct.resize(size);
				for (int a = 0; a < size; a++) {
					elemType->Serialize(s, &ct[a]);
				}
			}
		}
		std::string GetName() const { return elemType->GetName() + "[]"; }
		size_t GetSize() const { return sizeof(T); }
	};

	class StaticArrayBaseType : public IType
	{
	public:
		boost::shared_ptr<IType> elemType;
		int size, elemSize;

		StaticArrayBaseType(boost::shared_ptr<IType> et, int Size, int ElemSize)
			: elemType(et), size(Size), elemSize(ElemSize) {}
		~StaticArrayBaseType() {}

		std::string GetName() const;
		size_t GetSize() const { return size * elemSize; }
	};

	template<typename T, int Size>
	class StaticArrayType : public StaticArrayBaseType
	{
	public:
		typedef T ArrayType[Size];
		StaticArrayType(boost::shared_ptr<IType> et)
			: StaticArrayBaseType(et, Size, sizeof(ArrayType)/Size) {}
		void Serialize(ISerializer* s, void* instance)
		{
			T* array = (T*)instance;
			for (int a = 0; a < Size; a++)
				elemType->Serialize(s, &array[a]);
		}
	};

	template<typename T>
	class BitArrayType : public IType
	{
	public:
		boost::shared_ptr<IType> elemType;

		BitArrayType(boost::shared_ptr<IType> et)
			: elemType(et) {}
		~BitArrayType() {}

		void Serialize(ISerializer* s, void* inst) {
			T* ct = (T*)inst;
			if (s->IsWriting()) {
				int size = (int)ct->size();
				s->SerializeInt(&size, sizeof(int));
				for (int a = 0; a < size; a++) {
					bool b = (*ct)[a];
					elemType->Serialize(s, &b);
				}
			} else {
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct->resize(size);
				for (int a = 0; a < size; a++) {
					bool b;
					elemType->Serialize(s, &b);
					(*ct)[a] = b;
				}
			}
		}
		std::string GetName() const { return elemType->GetName() + "[]"; }
		size_t GetSize() const { return sizeof(T); }
	};

	class IgnoredType : public IType
	{
	public:
		int size;
		IgnoredType(int Size) {size=Size;}
		~IgnoredType() {}

		void Serialize(ISerializer* s, void* instance)
		{
			for (int a=0;a<size;a++) {
				char c=0;
				s->Serialize(&c,1);
			}
		}
		std::string GetName() const
		{
			return "ignored";
		}
		size_t GetSize() const { return size; }
	};

	class BasicType : public IType
	{
	public:
		BasicType(const BasicTypeID ID, const size_t size_) : size(size_), id(ID) {}
		~BasicType() {}

		void Serialize(ISerializer* s, void* instance);
		std::string GetName() const;
		size_t GetSize() const;

		size_t size;
		BasicTypeID id;
	};
}

#endif
