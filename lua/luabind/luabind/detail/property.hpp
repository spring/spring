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


#ifndef LUABIND_PROPERTY_HPP_INCLUDED
#define LUABIND_PROPERTY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/identity.hpp>
#include <luabind/dependency_policy.hpp>

namespace luabind { namespace detail
{
    namespace mpl = boost::mpl;
  
	class object_rep;
/*
	template<class R, class T, class Policies>
	int get(R(T::*f)() const, T* obj, lua_State* L, Policies* policies)  { return returns<R>::call(f, obj, L, policies); }

	template<class R, class T, class Policies>
	int get(R(*f)(const T*), T* obj, lua_State* L, Policies* policies) { return returns<R>::call(f, obj, L, policies); }

	template<class R, class T, class Policies>
	int get(R(*f)(const T&), T* obj, lua_State* L, Policies* policies) { return returns<R>::call(f, obj, L, policies); }
*/

	template<class R, class C, class T, class Policies>
	int get(R(C::*f)() const, T* obj, lua_State* L, Policies* policies)  
	{ 
		return returns<R>::call(f, obj, L, policies); 
	}

	template<class R, class T, class U, class Policies>
	int get(R(*f)(T), U* obj, lua_State* L, Policies* policies) 
	{ 
		return returns<R>::call(f, obj, L, policies); 
	}

	template<class T, class F, class Policies>
	struct get_caller : Policies
	{
		get_caller() {}
		get_caller(const Policies& p): Policies(p) {}

		int operator()(lua_State* L, int pointer_offset, F f)
		{
			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			return get(f, (T*)0, L, static_cast<Policies*>(this));
		}
	};

	template<class R, class C, class T, class A1, class Policies>
	int set(R(C::*f)(A1), T* obj, lua_State* L, Policies* policies)  
	{ 
		return returns<void>::call(f, obj, L, policies); 
	}
/*
	template<class R, class T, class A1, class Policies>
	int set(void(*f)(T*, A1), T* obj, lua_State* L, Policies* policies) { return returns<void>::call(f, obj, L, policies); }

	template<class T, class A1, class Policies>
	int set(void(*f)(T&, A1), T* obj, lua_State* L, Policies* policies) { return returns<void>::call(f, obj, L, policies); }
*/

	template<class R, class T, class U, class A1, class Policies>
	int set(R(*f)(T, A1), U* obj, lua_State* L, Policies* policies) { return returns<void>::call(f, obj, L, policies); }

	template<class T, class F, class Policies>
	struct set_caller : Policies
	{
		int operator()(lua_State* L, int pointer_offset, F f)
		{
			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			// 3. value

			// and since call() expects it's first
			// parameter on index 2 we need to
			// remove the key-parameter (parameter 2).
			lua_remove(L, 2);
			return luabind::detail::set(f, (T*)0, L, static_cast<Policies*>(this));
		}
	};

	typedef int (*match_fun_ptr)(lua_State*, int);

	template<class T, class Policies>
	struct set_matcher
	{
		static int apply(lua_State* L, int index)
		{
			typedef typename find_conversion_policy<1, Policies>::type converter_policy;
			typedef typename mpl::apply_wrap2<converter_policy,T,lua_to_cpp>::type converter;
			return converter::match(L, LUABIND_DECORATE_TYPE(T), index);
		}
	};

	template<class T, class Param, class Policy>
	match_fun_ptr gen_set_matcher(void (*)(T, Param), Policy*)
	{
		return set_matcher<Param, Policy>::apply;
	}

	template<class T, class Param, class Policy>
	match_fun_ptr gen_set_matcher(void (T::*)(Param), Policy*)
	{
		return set_matcher<Param, Policy>::apply;
	}

	template<class T, class D, class Policies>
	struct auto_set : Policies
	{
		auto_set() {}
		auto_set(const Policies& p): Policies(p) {}

		int operator()(lua_State* L, int pointer_offset, D T::*member)
		{
			int nargs = lua_gettop(L);

			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			// 3. value
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
			class_rep* crep = obj->crep();

			void* raw_ptr;

			if (crep->has_holder())
				raw_ptr = crep->extractor()(obj->ptr());
			else
				raw_ptr = obj->ptr();

			T* ptr =  reinterpret_cast<T*>(static_cast<char*>(raw_ptr) + pointer_offset);

			typedef typename find_conversion_policy<1,Policies>::type converter_policy;
			typename mpl::apply_wrap2<converter_policy,D,lua_to_cpp>::type converter;
			ptr->*member = converter.apply(L, LUABIND_DECORATE_TYPE(D), 3);

			int nret = lua_gettop(L) - nargs;

			const int indices[] = { 1, nargs + nret, 3 };

			policy_list_postcall<Policies>::apply(L, indices);

			return nret;
		}
	};

	// if the input type is a value_converter, it will produce
	// a reference converter
	template<class ConverterPolicy, class D>
	struct make_reference_converter
	{
		typedef typename mpl::apply_wrap2<
			ConverterPolicy
		  , typename boost::add_reference<D>::type
		  , cpp_to_lua
		>::type type;
	};
	
	template<class T, class D, class Policies>
	struct auto_get : Policies
	{
		auto_get() {}
		auto_get(const Policies& p): Policies(p) {}

		int operator()(lua_State* L, int pointer_offset, D T::*member)
		{
			int nargs = lua_gettop(L);

			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
			class_rep* crep = obj->crep();

			void* raw_ptr;

			if (crep->has_holder())
				raw_ptr = crep->extractor()(obj->ptr());
			else
				raw_ptr = obj->ptr();

			T* ptr =  reinterpret_cast<T*>(static_cast<char*>(raw_ptr) + pointer_offset);

			typedef typename find_conversion_policy<0,Policies>::type converter_policy;
			typedef typename mpl::apply_wrap2<converter_policy,D,cpp_to_lua>::type converter1_t;

			// if the converter is a valua converer, return a reference instead
			typedef typename boost::mpl::eval_if<
				BOOST_DEDUCED_TYPENAME converter1_t::is_value_converter
				, make_reference_converter<converter_policy, D>
				, mpl::identity<converter1_t>
			>::type converter2_t;

			// If this yields a reference converter, the dependency policy
			// is automatically added
			typedef typename boost::mpl::if_<
				BOOST_DEDUCED_TYPENAME converter1_t::is_value_converter
				, policy_cons<dependency_policy<1, 0>, Policies>
				, Policies
			>::type policy_list;

			converter2_t converter;
			converter.apply(L, ptr->*member);

			int nret = lua_gettop(L) - nargs;

			const int indices[] = { 1, nargs + nret };

			policy_list_postcall<policy_list>::apply(L, indices);

			return nret;
		}
	};

}}

#endif // LUABIND_PROPERTY_HPP_INCLUDED

