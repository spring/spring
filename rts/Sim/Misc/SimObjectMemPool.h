/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_MEMPOOL_H
#define SIMOBJECT_MEMPOOL_H

#include <cassert>
#include <cstring> // memset
#include <deque>
#include <vector>

#include "System/UnorderedMap.hpp"
#include "System/SafeUtil.h"

template<size_t N> struct SimObjectMemPool {
public:
	void clear() {
		pages.clear();
		indcs.clear();
		table.clear();
	}
	void reserve(size_t n) {
		indcs.reserve(n);
		table.reserve(n);
	}

	template<typename T, typename... A> T* alloc(A&&... a) {
		T* p = nullptr;
		uint8_t* m = nullptr;

		static_assert(sizeof(T) <= N, "");

		if (indcs.empty()) {
			pages.emplace_back();

			m = &pages[pages.size() - 1][0];
			p = new (m) T(a...);

			table.emplace(p, pages.size() - 1);
		} else {
			m = &pages[indcs.back()][0];
			p = new (m) T(a...);

			table.emplace(p, indcs.back());
			indcs.pop_back();
		}

		return p;
	}

	template<typename T> void free(T*& p) {
		const auto pit = table.find(p); assert(pit != table.end());
		const size_t idx = pit->second; assert(idx != -1lu);

		indcs.push_back(idx);
		table.erase(pit);

		spring::SafeDestruct(p);
		std::memset(&pages[idx][0], 0, N);
	}

private:
	std::deque< uint8_t[N] > pages;
	std::vector<size_t> indcs;

	// <pointer, page index> (non-intrusive)
	spring::unsynced_map<void*, size_t> table;
};

#endif

