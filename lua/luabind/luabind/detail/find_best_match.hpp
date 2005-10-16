// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef LUABIND_FIND_BEST_MATCH_HPP_INCLUDED
#define LUABIND_FIND_BEST_MATCH_HPP_INCLUDED

#include <luabind/config.hpp>

#include <boost/limits.hpp>

namespace luabind { namespace detail
{

	// expects that a match function can be accesed through the iterator
	// as int match_fun(lua_State*)
	// returns true if it found a match better than the given. If it finds a better match match_index is
	// updated to contain the new index to the best match (this index now refers to the list given to
	// this call).
	// orep_size is supposed to tell the size of the actual structures that start and end points to
	// ambiguous is set to true if the match was ambiguous
	// min_match should be initialized to the currently best match value (the number of implicit casts
	// to get a perfect match). If there are no previous matches, set min_match to std::numeric_limits<int>::max()

	LUABIND_API bool find_best_match(lua_State* L, const detail::overload_rep_base* start, int num_overloads, size_t orep_size, bool& ambiguous, int& min_match, int& match_index, int num_params);
	LUABIND_API void find_exact_match(lua_State* L, const detail::overload_rep_base* start, int num_overloads, size_t orep_size, int cmp_match, int num_params, std::vector<const overload_rep_base*>& dest);

}}

#endif // LUABIND_FIND_BEST_MATCH_HPP_INCLUDED

