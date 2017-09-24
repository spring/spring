/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_UNORDERED_MAP_H_
#define _SPRING_UNORDERED_MAP_H_

#define USE_EMILIB_HASH_MAP
// boost's implementation also uses LL buckets; see boost/unordered/detail/buckets.hpp
// (we want open addressing, so google's {sparse,dense}_hash_map are better candidates)
// #define USE_BOOST_HASH_MAP


#ifndef USE_EMILIB_HASH_MAP
	#ifdef USE_BOOST_HASH_MAP
	#include <boost/unordered_map.hpp>

	namespace spring {
		using boost::unordered_map;
		using boost::unordered_multimap;
		using unsynced_map = boost::unordered_map;
	};

	#else
	#include <unordered_map>

	namespace spring {
		using std::unordered_map;
		using std::unordered_multimap;
		using unsynced_map = std::unordered_map;
	};

	#endif
#else
	#include "SpringHashMap.hpp"
	#include "SpringHash.h"

	// NOTE: no multimap, no emplace
	namespace spring {
		template<typename K, typename V, typename H = spring::synced_hash<K>, typename C = emilib::HashMapEqualTo<K>>
		using unordered_map = emilib::HashMap<K, V, H, C>;
		template<typename K, typename V, typename H = std::hash<K>, typename C = emilib::HashMapEqualTo<K>>
		using unsynced_map = emilib::HashMap<K, V, H, C>;
	};
#endif


namespace spring {
	// Synced unordered maps must be reconstructed (on reload)
	// since clear() may keep the container resized which will
	// lead to differences in iteration order and then desyncs
	template<typename C> void clear_unordered_map(C& cont) { cont = C(); }

	template<typename K, typename V>
	class unordered_bimap {
	public:
		typedef spring::unordered_map<K, V> kv_map_type;
		typedef spring::unordered_map<V, K> vk_map_type;

		const kv_map_type& first() const { return kv_map; }
		const vk_map_type& second() const { return vk_map; }

		unordered_bimap(const std::initializer_list< std::pair<K, V> >& list): kv_map(list)
		{
			vk_map.reserve(list.size());

			for (const auto& pair: list) {
				if (vk_map.find(pair.second) == vk_map.end()) {
					vk_map[pair.second] = pair.first;
				}
			}
		}

	private:
		const kv_map_type kv_map;
		      vk_map_type vk_map;
	};
};

#endif

