#ifndef CR_LIST_TYPE_IMPL_H
#define CR_LIST_TYPE_IMPL_H

#include <list>

#include "creg.h"

namespace creg {

	// IType implemention for std::list
	template<typename T>
	struct ListType : public IType
	{
		ListType (IType *t) { elemType = t; }
		~ListType () { delete elemType; }

		void Serialize (ISerializer *s, void *inst) {
			T& ct = *(T*)inst;
			if (s->IsWriting ()) {
				int size = (int)ct.size();
				s->SerializeInt (&size,sizeof(int));
				for (typename T::iterator it = ct.begin(); it!=ct.end(); ++it)
					elemType->Serialize (s, &*it);
			} else {
				int size;
				s->SerializeInt (&size, sizeof(int));
				ct.resize (size);
				for (typename T::iterator it = ct.begin(); it!=ct.end(); ++it)
					elemType->Serialize (s, &*it);
			}
		}
		std::string GetName() { return "list<" + elemType->GetName() + ">"; }

		IType *elemType;
	};


	// List type
	template<typename T>
	struct DeduceType < std::list <T> > {
		IType* Get () { 
			DeduceType<T> elemtype;
			return SAFE_NEW ListType < std::list<T> > (elemtype.Get());
		}
	};
};


#endif
