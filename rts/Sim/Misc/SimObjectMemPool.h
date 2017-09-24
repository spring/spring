/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_MEMPOOL_H
#define SIMOBJECT_MEMPOOL_H

#include <cassert>
#include <cstring> // memset
#include <array>
#include <deque>
#include <vector>

#include "System/UnorderedMap.hpp"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"

template<size_t S> struct DynMemPool {
public:
	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(sizeof(T) <= page_size(), "");
		// disabled, recursion is allowed
		// assert(!xtorCall());

		ctor_call_depth += 1;

		T* p = nullptr;
		uint8_t* m = nullptr;

		size_t i = 0;

		if (indcs.empty()) {
			pages.emplace_back();

			i = pages.size() - 1;
			m = pages[curr_page_index = i].data();
			p = new (m) T(std::forward<A>(a)...);
		} else {
			// must pop before ctor runs; objects can be created recursively
			i = spring::VectorBackPop(indcs);

			m = pages[curr_page_index = i].data();
			p = new (m) T(std::forward<A>(a)...);
		}

		table.emplace(p, i);

		ctor_call_depth -= 1;
		return p;
	}

	template<typename T> void free(T*& p) {
		assert(mapped(p));
		// assert(!xtorCall());

		dtor_call_depth += 1;

		const auto iter = table.find(p);
		const auto pair = std::pair<void*, size_t>{iter->first, iter->second};

		spring::SafeDestruct(p);
		std::memset(pages[pair.second].data(), 0, page_size());

		// must push after dtor runs, since that can trigger *another* ctor call
		// by proxy (~CUnit -> ~CObject -> DependentDied -> CommandAI::FinishCmd
		// -> CBuilderCAI::ExecBuildCmd -> UnitLoader::LoadUnit -> CUnit e.g.)
		indcs.push_back(pair.second);
		table.erase(pair.first);

		dtor_call_depth -= 1;
	}


	static constexpr size_t page_size() { return S; }

	size_t alloc_size() const { return (pages.size() * page_size()); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (indcs.size() * page_size()); } // size of number of pages that were freed and are awaiting reuse

	bool mapped(void* p) const { return (table.find(p) != table.end()); }
	bool alloced(void* p) const { return ((curr_page_index < pages.size()) && (&pages[curr_page_index][0] == p)); }

	bool ctorCall() const { return (ctor_call_depth > 0); }
	bool dtorCall() const { return (dtor_call_depth > 0); }
	bool xtorCall() const { return (ctorCall() || dtorCall()); }

	void clear() {
		pages.clear();
		indcs.clear();
		table.clear();

		ctor_call_depth = 0;
		dtor_call_depth = 0;
		curr_page_index = 0;
	}
	void reserve(size_t n) {
		indcs.reserve(n);
		table.reserve(n);
	}

private:
	std::deque<std::array<uint8_t,S>> pages;
	std::vector<size_t> indcs;

	// <pointer, page index> (non-intrusive)
	spring::unsynced_map<void*, size_t> table;

	size_t ctor_call_depth = 0;
	size_t dtor_call_depth = 0;
	size_t curr_page_index = 0;
};



// fixed-size version
template<size_t N, size_t S> struct StaticMemPool {
public:
	StaticMemPool() { clear(); }

	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(num_pages() != 0, "");
		static_assert(sizeof(T) <= page_size(), "");

		T* p = nullptr;
		uint8_t* m = nullptr;

		size_t i = 0;

		assert(can_alloc());
		ctor_call_depth += 1;

		if (free_page_count == 0) {
			i = used_page_count++;
			m = pages[curr_page_index = i].data();
			p = new (m) T(std::forward<A>(a)...);
		} else {
			i = indcs[--free_page_count];
			m = pages[curr_page_index = i].data();
			p = new (m) T(std::forward<A>(a)...);
		}

		ctor_call_depth -= 1;
		return p;
	}

	template<typename T> void free(T*& t) {
		uint8_t* m = reinterpret_cast<uint8_t*>(t);

		assert(can_free());
		assert(mapped(t));

		dtor_call_depth += 1;

		spring::SafeDestruct(t);
		std::memset(m, 0, page_size());

		// mark page as free
		indcs[free_page_count++] = base_offset(m) / page_size();

		dtor_call_depth -= 1;
	}


	static constexpr size_t num_pages() { return N; }
	static constexpr size_t page_size() { return S; }

	size_t alloc_size() const { return (used_page_count * page_size()); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (free_page_count * page_size()); } // size of number of pages that were freed and are awaiting reuse
	size_t total_size() const { return (num_pages() * page_size()); }
	size_t base_offset(const void* p) const { return (reinterpret_cast<const uint8_t*>(p) - reinterpret_cast<const uint8_t*>(&pages[0][0])); }

	bool mapped(const void* p) const { return (((base_offset(p) / page_size()) < total_size()) && ((base_offset(p) % page_size()) == 0)); }
	bool alloced(const void* p) const { return (&pages[curr_page_index][0] == p); }

	bool can_alloc() const { return (used_page_count < num_pages() || free_page_count > 0); }
	bool can_free() const { return (free_page_count < num_pages()); }

	bool ctorCall() const { return (ctor_call_depth > 0); }
	bool dtorCall() const { return (dtor_call_depth > 0); }
	bool xtorCall() const { return (ctorCall() || dtorCall()); }

	void reserve(size_t) {} // no-op
	void clear() {
		std::memset(pages.data(), 0, total_size());
		std::memset(indcs.data(), 0, num_pages());

		used_page_count = 0;
		free_page_count = 0;
		curr_page_index = 0;

		ctor_call_depth = 0;
		dtor_call_depth = 0;
	}

private:
	std::array<std::array<uint8_t, S>, N> pages;
	std::array<size_t, N> indcs;

	size_t used_page_count = 0;
	size_t free_page_count = 0; // indcs[fpc-1] is the last recycled page
	size_t curr_page_index = 0;

	size_t ctor_call_depth = 0;
	size_t dtor_call_depth = 0;
};

#endif

