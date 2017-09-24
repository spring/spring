/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONTAINER_UTIL_H
#define CONTAINER_UTIL_H

#include <algorithm>
#include <cassert>
#include <vector>

namespace spring {
	template<typename T, typename TV>
	static auto find(T& c, const TV& v) -> decltype(c.end())
	{
		return std::find(c.begin(), c.end(), v);
	}

	template<typename T, typename UnaryPredicate>
	static void map_erase_if(T& c, UnaryPredicate p)
	{
		for (auto it = c.begin(); it != c.end(); ) {
			if (p(*it)) it = c.erase(it);
			else ++it;
		}
	}



	template<typename T, typename P>
	static bool VectorEraseIf(std::vector<T>& v, const P& p)
	{
		auto it = std::find_if(v.begin(), v.end(), p);

		if (it == v.end())
			return false;

		*it = v.back();
		v.pop_back();
		return true;
	}

	template<typename T>
	static bool VectorErase(std::vector<T>& v, T e)
	{
		auto it = std::find(v.begin(), v.end(), e);

		if (it == v.end())
			return false;

		*it = v.back();
		v.pop_back();
		return true;
	}

	template<typename T, typename C>
	static bool VectorEraseUniqueSorted(std::vector<T>& v, const T& e, const C& c)
	{
		const auto iter = std::lower_bound(v.begin(), v.end(), e, c);

		if ((iter == v.end()) || (*iter != e))
			return false;

		for (size_t n = (iter - v.begin()); n < (v.size() - 1); n++) {
			std::swap(v[n], v[n + 1]);
		}

		v.pop_back();
		return true;
	}



	template<typename T>
	static bool VectorInsertUnique(std::vector<T>& v, T e, bool b = false)
	{
		// do not assume uniqueness, test for it
		if (b && std::find(v.begin(), v.end(), e) != v.end())
			return false;

		// assume caller knows best, skip the test
		assert(b || std::find(v.begin(), v.end(), e) == v.end());
		v.push_back(e);
		return true;
	}

	template<typename T, typename C>
	static bool VectorInsertUniqueSorted(std::vector<T>& v, const T& e, const C& c)
	{
		const auto iter = std::lower_bound(v.begin(), v.end(), e, c);

		if ((iter != v.end()) && (*iter == e))
			return false;

		v.push_back(e);

		for (size_t n = v.size() - 1; n > 0; n--) {
			if (c(v[n - 1], v[n]))
				break;

			std::swap(v[n - 1], v[n]);
		}

		return true;
	}



	// emulate C++17's emplace_back
	template<typename T, typename... A>
	static T& VectorEmplaceBack(std::vector<T>& v, A&&... a) { v.emplace_back(std::forward<A>(a)...); return (v.back()); }

	template<typename T>
	static T VectorBackPop(std::vector<T>& v) { const T e = v.back(); v.pop_back(); return e; }
};

#endif

