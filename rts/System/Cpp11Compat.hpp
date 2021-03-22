/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPP11COMPAT_H
#define CPP11COMPAT_H

#include <functional>

namespace spring {
	template <typename ArgumentType, typename ResultType> using unary_function = std::function< ResultType(ArgumentType) >;
	template <typename Arg1, typename Arg2, typename Result> using binary_function = std::function< Result(Arg1, Arg2) >;

	// https://en.cppreference.com/w/cpp/algorithm/random_shuffle
	template<class RandomIt, class RandomFunc>
	void random_shuffle(RandomIt first, RandomIt last, RandomFunc&& r)
	{
		typename std::iterator_traits<RandomIt>::difference_type i, n;
		n = last - first;
		for (i = n - 1; i > 0; --i) {
			std::swap(first[i], first[r(i + 1)]);
		}
	}
};

#endif //CPP11COMPAT_H