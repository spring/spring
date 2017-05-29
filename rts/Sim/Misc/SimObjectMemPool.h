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
		// disabled, recursion is allowed
		// assert(!xtorCall());

		ctorCallDepth += 1;

		T* p = nullptr;
		uint8_t* m = nullptr;

		size_t i = -1lu;

		if (indcs.empty()) {
			pages.emplace_back();

			i = pages.size() - 1;
			m = &pages[allocPageIndx = i][0];
			p = new (m) T(std::forward<A>(a)...);
		} else {
			// must pop before ctor runs; objects can be created recursively
			i = spring::VectorBackPop(indcs);

			m = &pages[allocPageIndx = i][0];
			p = new (m) T(std::forward<A>(a)...);
		}

		table.emplace(p, i);

		ctorCallDepth -= 1;
		return p;
	}

	template<typename T> void free(T*& p) {
		assert(mapped(p));
		// assert(!xtorCall());

		dtorCallDepth += 1;

		const auto iter = table.find(p);
		const auto pair = std::pair<void*, size_t>{iter->first, iter->second};

		spring::SafeDestruct(p);
		std::memset(&pages[pair.second][0], 0, N);

		// must push after dtor runs, since that can trigger *another* ctor call
		// by proxy (~CUnit -> ~CObject -> DependentDied -> CommandAI::FinishCmd
		// -> CBuilderCAI::ExecBuildCmd -> UnitLoader::LoadUnit -> CUnit e.g.)
		indcs.push_back(pair.second);
		table.erase(pair.first);

		dtorCallDepth -= 1;
	}

	constexpr size_t page_size() const { return N; }

	size_t total_size() const { return (pages.size() * page_size()); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (indcs.size() * page_size()); } // size of number of pages that were freed and are awaiting reuse

	bool mapped(void* p) const { return (table.find(p) != table.end()); }
	bool alloced(void* p) const { return ((allocPageIndx < pages.size()) && (&pages[allocPageIndx][0] == p)); }

	bool ctorCall() const { return (ctorCallDepth > 0); }
	bool dtorCall() const { return (dtorCallDepth > 0); }
	bool xtorCall() const { return (ctorCall() || dtorCall()); }

	void clear() {
		pages.clear();
		indcs.clear();
		table.clear();

		ctorCallDepth = 0;
		dtorCallDepth = 0;
		allocPageIndx = 0;
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

	size_t ctorCallDepth = 0;
	size_t dtorCallDepth = 0;
	size_t allocPageIndx = 0;
};

#endif

