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

#ifndef LUABIND_OVERLOAD_REP_HPP_INCLUDED
#define LUABIND_OVERLOAD_REP_HPP_INCLUDED

#include <luabind/config.hpp>

#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <vector>

#include <luabind/detail/overload_rep_base.hpp>

#include <luabind/detail/is_indirect_const.hpp>

#ifndef BOOST_MSVC
#include <luabind/detail/policy.hpp>
#endif

namespace luabind { namespace detail
{
	struct dummy_ {};

	// this class represents a specific overload of a member-function.
	struct overload_rep: public overload_rep_base
	{
		#define BOOST_PP_ITERATION_PARAMS_1 (4 \
			, (0, LUABIND_MAX_ARITY, <luabind/detail/overload_rep.hpp>, 1))
		#include BOOST_PP_ITERATE()

		bool operator==(const overload_rep& o)
		{
			if (o.m_const != m_const) return false;
			if (o.m_arity != m_arity) return false;
			if (o.m_params_.size() != m_params_.size()) return false;
			for (int i = 0; i < (int)m_params_.size(); ++i)
			{
				if (!(LUABIND_TYPE_INFO_EQUAL(m_params_[i], o.m_params_[i]))) 
					return false;
			}
			return true;
		}

		void set_fun(boost::function1<int, lua_State*> const& f) 
		{ call_fun = f; }

		void set_fun_static(boost::function1<int, lua_State*> const& f) 
		{ call_fun_static = f; }

		int call(lua_State* L, bool force_static_call) const;

		bool has_static() const { return !call_fun_static.empty(); }

	private:

		// this is the normal function pointer that may be a virtual
		boost::function1<int, lua_State*> call_fun;

		// this is the optional function pointer that is only set if
		// the first function pointer is virtual. This must always point
		// to a static function.
		boost::function1<int, lua_State*> call_fun_static;

		// the types of the parameter it takes
		std::vector<LUABIND_TYPE_INFO> m_params_;
		// is true if the overload is const (this is a part of the signature)
		bool m_const;
	};

}} // namespace luabind::detail

#endif // LUABIND_OVERLOAD_REP_HPP_INCLUDED

#elif BOOST_PP_ITERATION_FLAGS() == 1

#define LUABIND_PARAM(z, n, _) m_params_.push_back(LUABIND_TYPEID(A##n));
#define LUABIND_POLICY_DECL(z,n,offset) typedef typename detail::find_conversion_policy<n + offset, Policies>::type BOOST_PP_CAT(p,n);
#define LUABIND_ARITY(z,n,text) + BOOST_PP_CAT(p,n)::has_arg

		// overloaded template funtion that initializes the parameter list
		// called m_params and the m_arity member.
		template<class R, class T BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A), class Policies>
		overload_rep(R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), Policies*)
            : m_const(false)
		{
			m_params_.reserve(BOOST_PP_ITERATION());
			BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_PARAM, _)
			BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POLICY_DECL, 2)
			m_arity = 1 BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_ARITY, 0);
		}

		template<class R, class T BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A), class Policies>
		overload_rep(R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)) const, Policies*)
            : m_const(true)
		{
			m_params_.reserve(BOOST_PP_ITERATION());
			BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_PARAM, _)
			BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POLICY_DECL, 2)
			m_arity = 1 BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_ARITY, 0);
		}

		template<class R BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), class A), class Policies>
		overload_rep(R(*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), Policies*)
            : m_const(false/*is_indirect_const<T>::value*/)
		{
			m_params_.reserve(BOOST_PP_ITERATION());
			BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_PARAM, _)
			BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POLICY_DECL, 1)
			m_arity = 0 BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_ARITY, 0);
		}

#undef LUABIND_ARITY
#undef LUABIND_POLICY_DECL
#undef LUABIND_PARAM

#endif
