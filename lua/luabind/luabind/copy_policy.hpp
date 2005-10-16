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


#ifndef LUABIND_COPY_POLICY_HPP_INCLUDED
#define LUABIND_COPY_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>

#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <luabind/detail/policy.hpp>

namespace luabind { namespace detail {

	struct copy_pointer_to
	{
		template<class T>
		void apply(lua_State* L, const T* ptr)
		{
			if (ptr == 0) 
			{
				lua_pushnil(L);
				return;
			}

			class_registry* registry = class_registry::get_registry(L);

			class_rep* crep = registry->find_class(LUABIND_TYPEID(T));

			// if you get caught in this assert you are trying
			// to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			T* copied_obj = new T(*ptr);

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			// we send 0 as destructor since we know it will never be called
			new(obj) object_rep(copied_obj, crep, object_rep::owner, delete_s<T>::apply);

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};

	struct copy_reference_to
	{
		template<class T>
		void apply(lua_State* L, const T& ref)
		{
			class_registry* registry = class_registry::get_registry(L);
			class_rep* crep = registry->find_class(LUABIND_TYPEID(T));

			// if you get caught in this assert you are trying
			// to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			T* copied_obj = new T(ref);

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			// we send 0 as destructor since we know it will never be called
			new(obj) object_rep(copied_obj, crep, object_rep::owner, delete_s<T>::apply);

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};

	template<int N>
	struct copy_policy : conversion_policy<N>
	{
		struct only_accepts_pointers_or_references {};
		struct only_converts_from_cpp_to_lua {};

		static void precall(lua_State*, const index_map&) {}
		static void postcall(lua_State*, const index_map&) {}

		template<class T, class Direction>
		struct generate_converter
		{
			typedef typename boost::mpl::if_<boost::is_same<Direction, cpp_to_lua>
					, typename boost::mpl::if_<boost::is_pointer<T>
						,	copy_pointer_to
						,	typename boost::mpl::if_<boost::is_reference<T>
								,	copy_reference_to
								,	only_accepts_pointers_or_references>::type>::type
					, only_converts_from_cpp_to_lua>::type type;
		};
	};
}}

namespace luabind
{
	template<int N>
	detail::policy_cons<detail::copy_policy<N>, detail::null_type> 
	copy(boost::arg<N>) { return detail::policy_cons<detail::copy_policy<N>, detail::null_type>(); }
}

#endif // LUABIND_COPY_POLICY_HPP_INCLUDED

