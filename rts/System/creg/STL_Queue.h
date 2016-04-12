/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_QUEUE_TYPE_IMPL_H
#define CR_QUEUE_TYPE_IMPL_H

#include "creg_cond.h"

#include <queue>

#ifdef USING_CREG

namespace creg {
	// From http://stackoverflow.com/a/1385520
	// It's insane and I hope it works
	template <class T, class S, class C>
    S& Container(std::priority_queue<T, S, C>& q) {
        struct HackedQueue : private std::priority_queue<T, S, C> {
            static S& Container(std::priority_queue<T, S, C>& q) {
                return q.*&HackedQueue::c;
            }
		};
		return HackedQueue::Container(q);
	}

	// IType implemention for std::list
	template<class T, class S, class C>
	struct PQueueType : public IType
	{
		PQueueType(boost::shared_ptr<IType> t):elemType(t) {}
		~PQueueType() {}

		void Serialize(ISerializer* s, void* inst) {
			S& ct = Container(*(std::priority_queue<T, S, C>*)inst);
			if (s->IsWriting()) {
				int size = (int)ct.size();
				s->SerializeInt(&size,sizeof(int));
				for (typename S::iterator it = ct.begin(); it != ct.end(); ++it)
				{
					elemType->Serialize(s, &*it);
				}
			} else {
				ct.clear();
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct.resize(size);
				for (typename S::iterator it = ct.begin(); it != ct.end(); ++it)
				{
					elemType->Serialize(s, &*it);
				}
			}
		}
		std::string GetName() const { return "priority_queue<" + elemType->GetName() + ">"; }
		size_t GetSize() const { return sizeof(std::priority_queue<T, S, C>); }

		boost::shared_ptr<IType> elemType;
	};


	// List type
	template<class T, class S, class C>
	struct DeduceType< std::priority_queue<T, S, C> > {
		static boost::shared_ptr<IType> Get() {
			return boost::shared_ptr<IType>(new PQueueType<T, S, C>(DeduceType<T>::Get()));
		}
	};
}

#endif // USING_CREG

#endif // CR_QUEUE_TYPE_IMPL_H

