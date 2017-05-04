/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_MEMPOOL_H
#define SIMOBJECT_MEMPOOL_H

#include <cassert>
#include <deque>
#include <vector>

template<size_t N> struct SimObjectMemPool {
public:
	void clear() {
		pages.clear();
		indcs.clear();
	}
	void reserve(size_t n) {
		indcs.reserve(128);
	}

	template<typename T> T* alloc() {
		T* p = nullptr;
		uint8_t* m = nullptr;

		static_assert(sizeof(T) <= N, "");

		if (indcs.empty()) {
			pages.emplace_back();

			m = &pages[pages.size() - 1][0];
			p = new (m) T(pages.size() - 1);
		} else {
			m = &pages[indcs.back()][0];
			p = new (m) T(indcs.back());

			indcs.pop_back();
		}

		return p;
	}

	template<typename T> void free(T* ptr) {
		indcs.push_back(ptr->GetMemPoolIdx());
		ptr->~T();
	}

private:
	std::deque< uint8_t[N] > pages;
	std::vector<size_t> indcs;
};

#endif

