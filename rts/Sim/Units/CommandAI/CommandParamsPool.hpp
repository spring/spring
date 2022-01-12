/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_PARAMS_POOL_H
#define COMMAND_PARAMS_POOL_H

#include <cassert>
#include <algorithm>
#include <vector>

#include "System/creg/creg_cond.h"

template<typename T, size_t N, size_t S> struct TCommandParamsPool {
public:
	const T* GetPtr(unsigned int i, unsigned int j     ) const { assert(i < pages.size()); return (pages[i].data( ) + j); }
	//    T* GetPtr(unsigned int i, unsigned int j     )       { assert(i < pages.size()); return (pages[i].data( ) + j); }
	      T  Get   (unsigned int i, unsigned int j     ) const { assert(i < pages.size()); return (pages[i].at  (j)    ); }
	      T  Set   (unsigned int i, unsigned int j, T v)       { assert(i < pages.size()); return (pages[i].at  (j) = v); }

	size_t Push(unsigned int i, T v) {
		assert(i < pages.size());
		pages[i].push_back(v);
		return (pages[i].size());
	}

	void ReleasePage(unsigned int i) {
		assert(i < pages.size());
		indcs.push_back(i);
	}

	unsigned int AcquirePage() {
		if (indcs.empty()) {
			const size_t numIndices = indcs.size();

			pages.resize(std::max(N, pages.size() << 1));
			indcs.resize(std::max(N, indcs.size() << 1));

			const auto beg = indcs.begin() + numIndices;
			const auto end = indcs.end();

			// generate new indices
			std::for_each(beg, end, [&](const unsigned int& i) { indcs[&i - &indcs[0]] = &i - &indcs[0]; });
		}

		const unsigned int pageIndex = indcs.back();

		pages[pageIndex].clear();
		pages[pageIndex].reserve(S);

		indcs.pop_back();
		return pageIndex;
	}

private:
	std::vector< std::vector<T> > pages;
	std::vector<unsigned int> indcs;
};

typedef TCommandParamsPool<float, 256, 32> CommandParamsPool;


extern CommandParamsPool cmdParamsPool;

#endif

