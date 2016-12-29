/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_UNORDERED_SET_H_
#define _SPRING_UNORDERED_SET_H_

// #define USE_BOOST_HASH_SET


#ifdef USE_BOOST_HASH_SET
#include <boost/unordered_set.hpp>

namespace spring {
	using boost::unordered_set;
	using boost::unordered_multiset;
};

#else
#include <unordered_set>

namespace spring {
	using std::unordered_set;
	using std::unordered_multiset;
};

#endif


#endif

