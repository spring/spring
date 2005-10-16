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


#if !BOOST_PP_IS_ITERATING

#ifndef LUABIND_CALL_HPP_INCLUDED
#define LUABIND_CALL_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/bool.hpp>

#include <luabind/config.hpp>
#include <luabind/detail/policy.hpp>
#include <luabind/yield_policy.hpp>

#define LUABIND_DECL(z, n, off) \
	typedef typename find_conversion_policy< \
		n + off \
	  , Policies \
	>::type BOOST_PP_CAT(converter_policy,n); \
	typename BOOST_PP_CAT(converter_policy,n)::template generate_converter< \
		A##n \
	  , lua_to_cpp \
	>::type BOOST_PP_CAT(c,n);

#define LUABIND_ADD_INDEX(z,n,text) \
	+ BOOST_PP_CAT(converter_policy,n)::has_arg

#define LUABIND_INDEX_MAP(z,n,text) \
	text BOOST_PP_REPEAT(n, LUABIND_ADD_INDEX, _)

#define LUABIND_PARAMS(z,n,text) \
	BOOST_PP_CAT(c,n).apply( \
		L \
	  , LUABIND_DECORATE_TYPE(A##n) \
	  , LUABIND_INDEX_MAP(_,n,text) \
	)

#define LUABIND_POSTCALL(z,n,text) \
	BOOST_PP_CAT(c,n).converter_postcall( \
		L \
	  , LUABIND_DECORATE_TYPE(A##n) \
	  , LUABIND_INDEX_MAP(_,n,text) \
	);

namespace luabind { namespace detail
{
	template<class Policies>
	struct maybe_yield
	{
		static inline int apply(lua_State* L, int nret)
		{
			return ret(L, nret, boost::mpl::bool_<has_yield<Policies>::value>());
		}

		static inline int ret(lua_State* L, int nret, boost::mpl::bool_<true>)
		{
			return lua_yield(L, nret);
		}

		static inline int ret(lua_State*, int nret, boost::mpl::bool_<false>)
		{
			return nret;
		}
	};

	template<class Class, class WrappedClass>
	struct most_derived
	{
		typedef typename boost::mpl::if_<
			boost::is_base_and_derived<Class, WrappedClass>
		  , WrappedClass
		  , Class
		>::type type;
	};

	template<class T>
	struct returns
	{
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>, 1))
		#include BOOST_PP_ITERATE()
	};

	template<>
	struct returns<void>
	{
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>, 2))
		#include BOOST_PP_ITERATE()
	};

	#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>, 3))
	#include BOOST_PP_ITERATE()
}}

#undef LUABIND_DECL
#undef LUABIND_PARAMS
#undef LUABIND_POSTCALL
#undef LUABIND_ADD_INDEX
#undef LUABIND_INDEX_MAP

#endif // LUABIND_CALL_HPP_INCLUDED

#elif BOOST_PP_ITERATION_FLAGS() == 1

	template<
		class C
	  , class WrappedClass
	  , class Policies
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	static int call(
		T(C::*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A))
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		int nargs = lua_gettop(L);

		typedef typename most_derived<C, WrappedClass>::type self_type;
		pointer_converter<lua_to_cpp> self_cv;

		typedef typename find_conversion_policy<0, Policies>::type converter_policy_ret;
		typename converter_policy_ret::template generate_converter<T, cpp_to_lua>::type converter_ret;

		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, 2)

		converter_ret.apply(
			L
		  , (self_cv.apply(L, LUABIND_DECORATE_TYPE(self_type*), 1)->*f)
		(
			BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, 2)
		));

		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, 2)

		int nret = lua_gettop(L) - nargs;

		const int indices[] =
		{
			nargs + nret // result
		  , 1 // self
			BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, 2)
		};

		policy_list_postcall<Policies>::apply(L, indices);

		return maybe_yield<Policies>::apply(L, nret);
	}

	template<
		class C
	  , class WrappedClass
	  , class Policies 
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	static int call(
		T(C::*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)) const
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		int nargs = lua_gettop(L);

		typedef typename most_derived<C, WrappedClass>::type self_type;
		const_pointer_converter<lua_to_cpp> self_cv;

		typedef typename find_conversion_policy<0, Policies>::type converter_policy_ret;
		typename converter_policy_ret::template generate_converter<T, cpp_to_lua>::type converter_ret;

		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, 2)

		converter_ret.apply(
			L
		  , (self_cv.apply(L, LUABIND_DECORATE_TYPE(self_type const*), 1)->*f)
		(
			BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, 2)
		));

		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, 2)
		int nret = lua_gettop(L) - nargs;

		const int indices[] =
		{
			nargs + nret // result
		  , 1 // self
			BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, 2)
		};

		policy_list_postcall<Policies>::apply(L, indices);

		return maybe_yield<Policies>::apply(L, nret);
	}

	template<
		class WrappedClass
	  , class Policies
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	static int call(
		T(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A))
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		int nargs = lua_gettop(L);
		typedef typename find_conversion_policy<0, Policies>::type converter_policy_ret;
		typename converter_policy_ret::template generate_converter<T, cpp_to_lua>::type converter_ret;
		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, 1)
		converter_ret.apply(L, f
		(
			BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, 1)
		));

		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, 1)
	
		int nret = lua_gettop(L) - nargs;

		const int indices[] =
		{
			nargs + nret // result
			BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, 1)
		};

		policy_list_postcall<Policies>::apply(L, indices);

		return maybe_yield<Policies>::apply(L, nret);
	}

#elif BOOST_PP_ITERATION_FLAGS() == 2

	template<
		class C
	  , class WrappedClass
	  , class Policies 
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) 
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	static int call(
		void(C::*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A))
	  , WrappedClass*
	  , lua_State* L
	  , Policies* const)
	{
		int nargs = lua_gettop(L);
		L = L; // L is used, but metrowerks compiler seem to warn about it before expanding the macros

		typedef typename most_derived<C, WrappedClass>::type self_type;
		pointer_converter<lua_to_cpp> self_cv;

		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, 2)
		(self_cv.apply(L, LUABIND_DECORATE_TYPE(self_type*), 1)->*f)
		(
			BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, 2)
		);
		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, 2)

		int nret = lua_gettop(L) - nargs;

		const int indices[] =
		{
			nargs + nret // result
		  , 1 // self
			BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, 2)
		};

		policy_list_postcall<Policies>::apply(L, indices);

		return maybe_yield<Policies>::apply(L, nret);
	}

	template<
		class C
	  , class WrappedClass
	  , class Policies 
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) 
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	static int call(
		void(C::*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)) const
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		int nargs = lua_gettop(L);
		L = L; // L is used, but metrowerks compiler seem to warn about it before expanding the macros

		typedef typename most_derived<C, WrappedClass>::type self_type;
		const_pointer_converter<lua_to_cpp> self_cv;
		
		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, 2)
		(self_cv.apply(L, LUABIND_DECORATE_TYPE(self_type const*), 1)->*f)
		(
			BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, 2)
		);
		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, 2)

		int nret = lua_gettop(L) - nargs;

		const int indices[] =
		{
			nargs + nret // result
		  , 1 // self
			BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, 2)
		};

		policy_list_postcall<Policies>::apply(L, indices);

		return maybe_yield<Policies>::apply(L, nret);
	}

	template<
		class WrappedClass
	  , class Policies 
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	static int call(
		void(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A))
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		int nargs = lua_gettop(L);
		L = L; // L is used, but metrowerks compiler seem to warn about it before expanding the macros
		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, 1)
		f(
			BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, 1)
		);

		int nret = lua_gettop(L) - nargs;

		const int indices[] =
		{
			nargs + nret /* result */
			BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, 1)
		};
		BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, 1)

		policy_list_postcall<Policies>::apply(L, indices);

		return maybe_yield<Policies>::apply(L, nret);
	}

#elif BOOST_PP_ITERATION_FLAGS() == 3

	template<
		class WrappedClass
	  , class Policies
	  , class R 
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	int call(
		R(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A))
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		return returns<R>::call(f, (WrappedClass*)0, L, (Policies*)0);
	}

	template<
		class T
	  , class WrappedClass
	  , class Policies
	  , class R 
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	int call(
		R(T::*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A))
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		return returns<R>::call(f, (WrappedClass*)0, L, (Policies*)0);
	}

	template<
		class T
	  , class WrappedClass
	  , class Policies
	  , class R
		BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)
	>
	int call(
		R(T::*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)) const
	  , WrappedClass*
	  , lua_State* L
	  , Policies const*)
	{
		return returns<R>::call(f, (WrappedClass*)0, L, (Policies*)0);
	}

#endif

