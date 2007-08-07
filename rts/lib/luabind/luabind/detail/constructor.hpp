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

#ifndef LUABIND_CONSTRUCTOR_HPP_INCLUDED
#define LUABIND_CONSTRUCTOR_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/comma_if.hpp>

#include <boost/mpl/apply_wrap.hpp>

#include <luabind/config.hpp>
#include <luabind/wrapper_base.hpp>
#include <luabind/detail/policy.hpp>
#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/call_member.hpp>
#include <luabind/wrapper_base.hpp>
#include <luabind/weak_ref.hpp>

namespace luabind { namespace detail
{
	template<int N>
	struct constructor_helper;

	namespace mpl = boost::mpl;
	
	template<int N>
	struct wrapped_constructor_helper;
	
	#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/constructor.hpp>, 1))
	#include BOOST_PP_ITERATE()

	template<class T, class Policies, class ConstructorSig>
	struct construct_class
	{
		inline static void* apply(lua_State* L, weak_ref const& ref)
		{
			typedef constructor_helper<ConstructorSig::arity> helper;
			return helper::execute(
                L
			  , ref
              , (T*)0
              , (ConstructorSig*)0
              , (Policies*)0
            );
		}
	};

	template<class T, class W, class Policies, class ConstructorSig>
	struct construct_wrapped_class
	{
		inline static void* apply(lua_State* L, weak_ref const& ref)
		{
			typedef wrapped_constructor_helper<ConstructorSig::arity> helper;
			return helper::execute(
                L
              , ref
              , (T*)0
              , (W*)0
              , (ConstructorSig*)0
              , (Policies*)0
            );
		}
	};

}}

#endif // LUABIND_CONSTRUCTOR_HPP_INCLUDED


#elif BOOST_PP_ITERATION_FLAGS() == 1


#define LUABIND_DECL(z, n, text) \
	typedef typename find_conversion_policy<n+1,Policies>::type BOOST_PP_CAT(converter_policy,n); \
\
	typename mpl::apply_wrap2< \
		BOOST_PP_CAT(converter_policy,n), BOOST_PP_CAT(A,n), lua_to_cpp \
	>::type BOOST_PP_CAT(c,n);

#define LUABIND_PARAM(z,n,text) \
	BOOST_PP_CAT(c,n).apply(L, LUABIND_DECORATE_TYPE(BOOST_PP_CAT(A,n)), n + 2)

	template<>
	struct constructor_helper<BOOST_PP_ITERATION()>
	{
        template<class T, class Policies, BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
        static T* execute(
            lua_State* L
          , weak_ref const&
          , T*
          , constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY,A)>*
          , Policies*)
        {
            BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
            return new T(BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAM, _));
        }
/*
		template<class T>
		struct apply
		{
			template<class Policies, BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
			static T* call(lua_State* L, const constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY,A)>*, const Policies*)
			{
				// L is used, but the metrowerks compiler warns about this before expanding the macros
				L = L;
				BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
				return new T(BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAM, _));
			}
		};*/
	};

	template<>
	struct wrapped_constructor_helper<BOOST_PP_ITERATION()>
	{
        template<class T, class W, class Policies, BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
        static T* execute(
            lua_State* L
          , weak_ref const& ref
          , T*
          , W*
          , constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY,A)>*
          , Policies*)
        {
            BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
            W* result = new W(BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAM, _));
            static_cast<weak_ref&>(detail::wrap_access::ref(*result)) = ref;
            return result;
        }
/*
        template<class T>
		struct apply
		{
			template<class Policies, BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
			static T* call(lua_State* L, weak_ref const& ref, const constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY,A)>*, const Policies*)
			{
				BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
				T* o = new T(BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAM, _));
				static_cast<weak_ref&>(detail::wrap_access::ref(*o)) = ref;
				return o;
			}
		};*/
	};


#undef LUABIND_PARAM
#undef LUABIND_DECL

#endif // LUABIND_CONSTRUCTOR_HPP_INCLUDED

