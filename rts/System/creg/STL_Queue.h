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
		PQueueType() : IType(sizeof(std::priority_queue<T, S, C>)) { }
		~PQueueType() { }

		void Serialize(ISerializer* s, void* inst) {
			S& ct = Container(*(std::priority_queue<T, S, C>*)inst);
			if (s->IsWriting()) {
				int size = (int)ct.size();
				s->SerializeInt(&size,sizeof(int));
				for (typename S::iterator it = ct.begin(); it != ct.end(); ++it)
				{
					DeduceType<T>::Get()->Serialize(s, &*it);
				}
			} else {
				ct.clear();
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct.resize(size);
				for (typename S::iterator it = ct.begin(); it != ct.end(); ++it)
				{
					DeduceType<T>::Get()->Serialize(s, &*it);
				}
			}
		}
		std::string GetName() const { return "priority_queue<" + DeduceType<T>::Get()->GetName() + ">"; }
	};


	// List type
	template<class T, class S, class C>
	struct DeduceType< std::priority_queue<T, S, C> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new PQueueType<T, S, C>());
		}
	};
}

#endif // USING_CREG

#endif // CR_QUEUE_TYPE_IMPL_H

