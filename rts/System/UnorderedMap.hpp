/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_UNORDERED_MAP_H_
#define _SPRING_UNORDERED_MAP_H_

// boost's implementation also uses LL buckets; see boost/unordered/detail/buckets.hpp
// (we want open addressing, so google's {sparse,dense}_hash_map are better candidates)
// #define USE_BOOST_HASH_MAP


#ifdef USE_BOOST_HASH_MAP
#include <boost/unordered_map.hpp>

namespace spring {
	using boost::unordered_map;
	using boost::unordered_multimap;
};

#else
#include <unordered_map>

namespace spring {
	using std::unordered_map;
	using std::unordered_multimap;
};

#endif


#endif

