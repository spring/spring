/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_DEQUE_TYPE_IMPL_H
#define CR_DEQUE_TYPE_IMPL_H

#include "creg_cond.h"

#include <deque>

#ifdef USING_CREG

namespace creg
{
	/// Deque type (uses vector implementation)
	template<typename T>
	struct DeduceType< std::deque <T> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new DynamicArrayType< std::deque<T> >());
		}
	};
}

#endif // USING_CREG

#endif // CR_DEQUE_TYPE_IMPL_H
