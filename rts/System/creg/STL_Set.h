/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_SET_TYPE_IMPL_H
#define CR_SET_TYPE_IMPL_H

#include "creg_cond.h"

#ifdef USING_CREG

#if defined(_MSC_VER)
	#include <hash_set>
	#define SPRING_HASH_SET stdext::hash_set
#elif defined(_LIBCPP_VERSION)
	#include <unordered_set>
	#define SPRING_HASH_SET std::unordered_set
#elif __GNUG__
	#include <unordered_set>
	#define SPRING_HASH_SET std::unordered_set
#else
	#error Unsupported compiler
#endif

#include <set>

namespace creg
{
	// set/multiset - this template assumes the type is copyable
	template<typename T>
	class SetType : public IType
	{
		boost::shared_ptr<IType> elemType;
	public:
		typedef typename T::iterator iterator;

		SetType(boost::shared_ptr<IType> elemType) : elemType(elemType) {}
		~SetType() {}

		void Serialize(ISerializer* s, void* instance)
		{
			T& ct = *(T*)instance;
			if (s->IsWriting()) {
				int size = ct.size();
				s->SerializeInt(&size, sizeof(int));
				for (iterator i = ct.begin(); i != ct.end(); ++i)
					elemType->Serialize(s,(void*) &*i);
			} else {
				int size;
				s->SerializeInt(&size, sizeof(int));
				for (int i = 0; i < size; i++) {
					typename T::value_type v;
					elemType->Serialize(s, &v);
					ct.insert(v);
				}
			}
		}
		std::string GetName() const { return "set<" + elemType->GetName() + ">"; }
		size_t GetSize() const { return sizeof(T); }
	};


	// Set type
	template<typename T, typename C>
	struct DeduceType<std::set<T, C> > {
		static boost::shared_ptr<IType> Get() {
			DeduceType<T> elemtype;
			return boost::shared_ptr<IType>(new SetType<std::set<T, C> >(elemtype.Get()));
		}
	};
	// Multiset
	template<typename T>
	struct DeduceType<std::multiset<T> > {
		static boost::shared_ptr<IType> Get() {
			DeduceType<T> elemtype;
			return boost::shared_ptr<IType>(new SetType<std::multiset<T> >(elemtype.Get()));
		}
	};
	// Hash set
	template<typename T>
	struct DeduceType<SPRING_HASH_SET<T> > {
		static boost::shared_ptr<IType> Get() {
			DeduceType<T> elemtype;
			return boost::shared_ptr<IType>(new SetType<SPRING_HASH_SET<T> >(elemtype.Get()));
		}
	};
}

#endif // USING_CREG

#endif // CR_SET_TYPE_IMPL_H

