#ifndef CR_LIST_TYPE_IMPL_H
#define CR_LIST_TYPE_IMPL_H

#include <list>

#include "creg_cond.h"

namespace creg {

	// IType implemention for std::list
	template<typename T>
	struct ListType : public IType
	{
		ListType (boost::shared_ptr<IType> t):elemType(t) {}
		~ListType () {}

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

		boost::shared_ptr<IType> elemType;
	};


	// List type
	template<typename T>
	struct DeduceType < std::list <T> > {
		boost::shared_ptr<IType> Get () {
			DeduceType<T> elemtype;
			return boost::shared_ptr<IType>(new ListType < std::list<T> > (elemtype.Get()));
		}
	};
};


#endif
