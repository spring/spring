/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_SET_TYPE_IMPL_H
#define CR_SET_TYPE_IMPL_H

#include "creg_cond.h"
#include "System/UnorderedSet.hpp"

#include <set>

#ifdef USING_CREG

namespace creg
{
	// set/multiset - this template assumes the type is copyable
	template<typename T>
	class SetType : public IType
	{
		std::shared_ptr<IType> elemType;
	public:
		typedef typename T::iterator iterator;

		SetType() : IType(sizeof(T)), elemType(DeduceType<typename T::value_type>::Get()) { }
		~SetType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			T& ct = *(T*)instance;
			if (s->IsWriting()) {
				int size = ct.size();
				s->SerializeInt(&size, sizeof(int));
				for (iterator i = ct.begin(); i != ct.end(); ++i)
					elemType->Serialize(s,(void*) &*i);
			} else {
				ct.clear();
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
	};


	// Set type
	template<typename T, typename C>
	struct DeduceType<std::set<T, C> > {
		static std::shared_ptr<IType> Get() {
			return std::shared_ptr<IType>(new SetType<std::set<T, C> >());
		}
	};
	// Multiset
	template<typename T>
	struct DeduceType<std::multiset<T> > {
		static std::shared_ptr<IType> Get() {
			return std::shared_ptr<IType>(new SetType<std::multiset<T> >());
		}
	};
	// Hash set
	template<typename T>
	struct DeduceType<spring::unordered_set<T> > {
		static std::shared_ptr<IType> Get() {
			return std::shared_ptr<IType>(new SetType<spring::unordered_set<T> >());
		}
	};
	// Unsynced Hash set
	template<typename T>
	struct DeduceType<spring::unsynced_set<T> > {
		static std::shared_ptr<IType> Get() {
			return std::shared_ptr<IType>(new SetType<spring::unsynced_set<T> >());
		}
	};
}

#endif // USING_CREG

#endif // CR_SET_TYPE_IMPL_H

