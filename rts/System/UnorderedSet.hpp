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
		using unsynced_set = boost::unordered_set;
	};

	#else
	#include <unordered_set>

	namespace spring {
		using std::unordered_set;
		using std::unordered_multiset;
		using unsynced_set = std::unordered_set;
	};

	#endif
#else
	#include "SpringHashSet.hpp"
	#include "SpringHash.h"

	// NOTE: no multiset
	namespace spring {
		template<typename K, typename H = spring::synced_hash<K>, typename C = emilib::HashSetEqualTo<K>>
		using unordered_set = emilib::HashSet<K, H, C>;
		template<typename K, typename H = std::hash<K>, typename C = emilib::HashSetEqualTo<K>>
		using unsynced_set = emilib::HashSet<K, H, C>;
	};
#endif


namespace spring {
	// Synced unordered sets must be reconstructed (on reload)
	// since clear() may keep the container resized which will
	// lead to differences in iteration order and then desyncs
	template<typename C> void clear_unordered_set(C& cont) { cont = C(); }
};


#endif

