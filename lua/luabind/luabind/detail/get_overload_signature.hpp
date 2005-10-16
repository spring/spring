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

#ifndef LUABIND_GET_OVERLOAD_SIGNATURE_HPP_INCLUDED
#define LUABIND_GET_OVERLOAD_SIGNATURE_HPP_INCLUDED

#include <string>

namespace luabind { namespace detail
{

	template<class It>
	std::string get_overload_signatures(lua_State* L, It start, It end, std::string name)
	{
		std::string s;
		for (; start != end; ++start)
		{
			s += name;
			start->get_signature(L, s);
			s += "\n";
		}
		return s;
	}


#ifndef LUABIND_NO_ERROR_CHECKING

	std::string get_overload_signatures_candidates(lua_State* L, std::vector<const overload_rep_base*>::iterator start, std::vector<const overload_rep_base*>::iterator end, std::string name);

#endif

}}

#endif // LUABIND_GET_OVERLOAD_SIGNATURE_HPP_INCLUDED

