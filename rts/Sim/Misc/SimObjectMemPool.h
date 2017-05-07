/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_MEMPOOL_H
#define SIMOBJECT_MEMPOOL_H

#include <cassert>
#include <cstring> // memset
#include <deque>
#include <vector>

#include "System/UnorderedMap.hpp"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"

template<size_t N> struct SimObjectMemPool {
public:
	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(sizeof(T) <= N, "");

		T* p = nullptr;
		uint8_t* m = nullptr;

		size_t i = -1lu;

		if (indcs.empty()) {
			pages.emplace_back();

			i = pages.size() - 1;
			m = &pages[i][0];
			p = new (m) T(std::forward<A>(a)...);

			table.emplace(p, i);
		} else {
			i = spring::VectorBackPop(indcs);
			m = &pages[i][0];
			p = new (m) T(std::forward<A>(a)...);

			table.emplace(p, i);
			// must pop before ctor runs; objects can be created recursively
			// indcs.pop_back();
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

	template<typename T> bool mapped(const T* p) const { return (table.find(p) != table.end()); }

	void clear() {
		pages.clear();
		indcs.clear();
		table.clear();
	}
	void reserve(size_t n) {
		indcs.reserve(n);
		table.reserve(n);
	}

private:
	std::deque< uint8_t[N] > pages;
	std::vector<size_t> indcs;

	// <pointer, page index> (non-intrusive)
	spring::unsynced_map<void*, size_t> table;
};

#endif

