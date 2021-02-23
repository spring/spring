/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPP11COMPAT_H
#define CPP11COMPAT_H

#include <functional>

namespace spring {
	template <typename ArgumentType, typename ResultType> using unary_function = std::function< ResultType(ArgumentType) >;
	template <typename Arg1, typename Arg2, typename Result> using binary_function = std::function< Result(Arg1, Arg2) >;
};

#endif //CPP11COMPAT_H