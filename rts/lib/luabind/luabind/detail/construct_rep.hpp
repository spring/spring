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


#ifndef LUABIND_CONSTRUCT_REP_HPP_INCLUDED
#define LUABIND_CONSTRUCT_REP_HPP_INCLUDED

#include <luabind/config.hpp>

#include <vector>
#include <string>

#include <boost/preprocessor/enum_params.hpp>
#include <boost/function/function1.hpp>
#include <boost/function/function2.hpp>

//#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/overload_rep_base.hpp>
#include <luabind/weak_ref.hpp>

namespace luabind { namespace detail
{


	struct construct_rep
	{
		struct overload_t: public overload_rep_base
		{
			overload_t(): wrapped_construct_fun(0) 
			{
			}

			typedef void*(*construct_ptr)(lua_State*, weak_ref const&);
			typedef void*(*wrapped_construct_ptr)(lua_State*, weak_ref const&);
			typedef void(*get_signature_ptr)(lua_State*, std::string&);

			inline void set_constructor(construct_ptr f) { construct_fun = f; }
			inline void set_wrapped_constructor(wrapped_construct_ptr f) { wrapped_construct_fun = f; }

			inline void* construct(lua_State* L, weak_ref const& backref) 
			{ 
				return construct_fun(L, backref); 
			}

			inline void* construct_wrapped(lua_State* L, weak_ref const& ref) { return wrapped_construct_fun(L, ref); } 
			inline bool has_wrapped_construct() { return wrapped_construct_fun != 0; }

			inline void set_arity(int arity) { m_arity = arity; }

		private:

			construct_ptr construct_fun;
			wrapped_construct_ptr wrapped_construct_fun;

		};

		void swap(construct_rep& x)
		{
			std::swap(x.overloads, overloads);
		}

		std::vector<overload_t> overloads;
	};

}}

#endif // LUABIND_CONSTRUCT_REP_HPP_INCLUDED
