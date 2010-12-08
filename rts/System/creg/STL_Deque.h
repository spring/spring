/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_DEQUE_TYPE_IMPL_H
#define CR_DEQUE_TYPE_IMPL_H

#include "creg_cond.h"

#ifdef USING_CREG

#include <deque>

namespace creg
{
	// Deque type (uses vector implementation)
	template<typename T>
	struct DeduceType < std::deque <T> > {
		boost::shared_ptr<IType> Get () {
			DeduceType<T> elemtype;
			return boost::shared_ptr<IType>(new DynamicArrayType < std::deque<T> > (elemtype.Get()));
		}
	};
};

#endif // USING_CREG

#endif // CR_DEQUE_TYPE_IMPL_H
