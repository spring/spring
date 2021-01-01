/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_LIST_TYPE_IMPL_H
#define CR_LIST_TYPE_IMPL_H

#include "creg_cond.h"

#include <list>

#ifdef USING_CREG

namespace creg {

	// IType implemention for std::list
	template<typename T>
	struct ListType : public IType
	{
		ListType() : IType(sizeof(T)) {}
		~ListType() {}

		void Serialize(ISerializer* s, void* inst) {
			T& ct = *(T*)inst;
			if (s->IsWriting()) {
				int size = (int)ct.size();
				s->SerializeInt(&size,sizeof(int));
				for (typename T::iterator it = ct.begin(); it != ct.end(); ++it)
				{
					DeduceType<T>::Get()->Serialize(s, &*it);
				}
			} else {
				ct.clear();
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct.resize(size);
				for (typename T::iterator it = ct.begin(); it != ct.end(); ++it)
				{
					DeduceType<T>::Get()->Serialize(s, &*it);
				}
			}
		}
		std::string GetName() const { return "list<" + DeduceType<T>::Get()->GetName() + ">"; }
	};


	// List type
	template<typename T>
	struct DeduceType< std::list<T> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new ListType< std::list<T> >());
		}
	};
}

#endif // USING_CREG

#endif // CR_LIST_TYPE_IMPL_H

