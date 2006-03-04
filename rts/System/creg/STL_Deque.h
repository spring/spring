#ifndef CR_DEQUE_TYPE_IMPL_H
#define CR_DEQUE_TYPE_IMPL_H

#include <deque>

namespace creg
{
	// Deque type (uses vector implementation)
	template<typename T>
	struct DeduceType < std::deque <T> > {
		IType* Get () { 
			DeduceType<T> elemtype;
			return new DynamicArrayType < std::deque<T> > (elemtype.Get());
		}
	};
};

#endif
