/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_UNORDERED_SET_H_
#define _SPRING_UNORDERED_SET_H_

#define USE_EMILIB_HASH_SET
// #define USE_BOOST_HASH_SET


#ifndef USE_EMILIB_HASH_SET
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
#else
	#include "SpringHashSet.hpp"

	// NOTE: no multiset
	namespace spring {
		template<typename K, typename H = std::hash<K>, typename C = emilib::HashSetEqualTo<K>>
		using unordered_set = emilib::HashSet<K, H, C>;
	};
#endif


#endif

