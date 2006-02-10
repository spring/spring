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

#ifndef LUABIND_FUNCTION_HPP_INCLUDED
#define LUABIND_FUNCTION_HPP_INCLUDED

#include <luabind/prefix.hpp>
#include <luabind/config.hpp>

#include <boost/config.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/apply_wrap.hpp>

#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/call_function.hpp>
#include <luabind/detail/get_overload_signature.hpp>
#include <luabind/detail/overload_rep_base.hpp>

#include <luabind/scope.hpp>

namespace luabind
{
	namespace detail
	{

		namespace mpl = boost::mpl;
	  
		namespace free_functions
		{

			struct overload_rep: public overload_rep_base
			{

#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 1))
#include BOOST_PP_ITERATE()

				inline bool operator==(const overload_rep& o) const
				{
					if (o.m_arity != m_arity) return false;
					if (o.m_params_.size() != m_params_.size()) return false;
					for (int i = 0; i < (int)m_params_.size(); ++i)
						if (!(LUABIND_TYPE_INFO_EQUAL(m_params_[i], o.m_params_[i]))) return false;
					return true;
				}

				typedef int(*call_ptr)(lua_State*, void(*)());

				inline void set_fun(call_ptr f) { call_fun = f; }
				inline int call(lua_State* L, void(*f)()) const { return call_fun(L, f); }

				// this is the actual function pointer to be called when this overload is invoked
				void (*fun)();

//TODO:		private: 

				call_ptr call_fun;

				// the types of the parameter it takes
				std::vector<LUABIND_TYPE_INFO> m_params_;

                char end;
			};

    		struct LUABIND_API function_rep
			{
				function_rep(const char* name): m_name(name) {}
				void add_overload(const free_functions::overload_rep& o);

				const std::vector<overload_rep>& overloads() const throw() { return m_overloads; }

				const char* name() const { return m_name; }

			private:
				const char* m_name;

				// this have to be write protected, since each time an overload is
				// added it has to be checked for existence. add_overload() should
				// be used.
				std::vector<free_functions::overload_rep> m_overloads;
			};



		// returns generates functions that calls function pointers

#define LUABIND_DECL(z, n, text) typedef typename find_conversion_policy<n + 1, Policies>::type BOOST_PP_CAT(converter_policy,n); \
		typename mpl::apply_wrap2< \
			BOOST_PP_CAT(converter_policy,n), BOOST_PP_CAT(A,n), lua_to_cpp \
		>::type BOOST_PP_CAT(c,n);

#define LUABIND_ADD_INDEX(z,n,text) + BOOST_PP_CAT(converter_policy,n)::has_arg
#define LUABIND_INDEX_MAP(z,n,text) 1 BOOST_PP_REPEAT(n, LUABIND_ADD_INDEX, _)
#define LUABIND_PARAMS(z,n,text) BOOST_PP_CAT(c,n).apply(L, LUABIND_DECORATE_TYPE(A##n), LUABIND_INDEX_MAP(_,n,_))
#define LUABIND_POSTCALL(z,n,text) BOOST_PP_CAT(c,n).converter_postcall(L, LUABIND_DECORATE_TYPE(A##n), LUABIND_INDEX_MAP(_,n,_));

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

		
			template<class T>
			struct returns
			{
				#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 2))
				#include BOOST_PP_ITERATE()
			};

			template<>
			struct returns<void>
			{
				#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 3))
				#include BOOST_PP_ITERATE()
			};

			#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 4))
			#include BOOST_PP_ITERATE()


#undef LUABIND_PARAMS
#undef LUABIND_DECL
#undef LUABIND_POSTCALL
#undef LUABIND_ADD_INDEX
#undef LUABIND_INDEX_MAP


			#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 5))
			#include BOOST_PP_ITERATE()


			template<class F, class Policies>
			struct function_callback_s
			{
				static inline int apply(lua_State* L, void(*fun)())
				{
					return free_functions::call(reinterpret_cast<F>(fun), L, static_cast<const Policies*>(0));
				}
			};

			template<class F, class Policies>
			struct match_function_callback_s
			{
				static inline int apply(lua_State* L)
				{
					F fptr = 0;
					return free_functions::match(fptr, L, static_cast<Policies*>(0));
				}

				static int callback(lua_State* L)
				{
					F fptr = 0;
					return free_functions::match(fptr, L, static_cast<Policies*>(0));
				}
			};

			LUABIND_API int function_dispatcher(lua_State* L);
		}
	}

	// deprecated
	template<class F, class Policies>
	void function(lua_State* L, const char* name, F f, const Policies& p)
	{
		module(L) [ def(name, f, p) ];
	}

	// deprecated
	template<class F>
	void function(lua_State* L, const char* name, F f)
	{
		luabind::function(L, name, f, detail::null_type());
	}

	namespace detail
	{
		template<class F, class Policies>
		struct function_commiter : detail::registration
		{
			function_commiter(const char* n, F f, const Policies& p)
				: m_name(n)
				, fun(f)
				, policies(p)
			{}

			virtual void register_(lua_State* L) const
			{
				detail::free_functions::overload_rep o(fun, static_cast<Policies*>(0));

				o.set_match_fun(&detail::free_functions::match_function_callback_s<F, Policies>::apply);
				o.set_fun(&detail::free_functions::function_callback_s<F, Policies>::apply);

#ifndef LUABIND_NO_ERROR_CHECKING
				o.set_sig_fun(&detail::get_free_function_signature<F>::apply);
#endif

				lua_pushstring(L, m_name);
				lua_gettable(L, -2);

				detail::free_functions::function_rep* rep = 0;
				if (lua_iscfunction(L, -1))
				{
					if (lua_getupvalue(L, -1, 2) != 0)
					{
						// check the magic number that identifies luabind's functions
						if (lua_touserdata(L, -1) == (void*)0x1337)
						{
							if (lua_getupvalue(L, -2, 1) != 0)
							{
								rep = static_cast<detail::free_functions::function_rep*>(lua_touserdata(L, -1));
								lua_pop(L, 1);
							}
						}
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1);

				if (rep == 0)
				{
					lua_pushstring(L, m_name);
					// create a new function_rep
					rep = static_cast<detail::free_functions::function_rep*>(lua_newuserdata(L, sizeof(detail::free_functions::function_rep)));
					new(rep) detail::free_functions::function_rep(m_name);

                    // STORE IN REGISTRY
                    lua_pushvalue(L, -1);
                    detail::ref(L);

					detail::class_registry* r = detail::class_registry::get_registry(L);
					assert(r && "you must call luabind::open() prior to any function registrations");
					detail::getref(L, r->lua_function());
					int ret = lua_setmetatable(L, -2);
					(void)ret;
					assert(ret != 0);

					// this is just a magic number to identify functions that luabind created
					lua_pushlightuserdata(L, (void*)0x1337);

					lua_pushcclosure(L, &free_functions::function_dispatcher, 2);
					lua_settable(L, -3);
				}

				rep->add_overload(o);
			}

			char const* m_name;
			F fun;
			Policies policies;
		};
	}

	template<class F, class Policies>
	scope def(const char* name, F f, const Policies& policies)
	{
		return scope(std::auto_ptr<detail::registration>(
			new detail::function_commiter<F,Policies>(name, f, policies)));
	}

	template<class F>
	scope def(const char* name, F f)
	{
		return scope(std::auto_ptr<detail::registration>(
			new detail::function_commiter<F,detail::null_type>(
				name, f, detail::null_type())));
	}

} // namespace luabind


#endif // LUABIND_FUNCTION_HPP_INCLUDED

#elif BOOST_PP_ITERATION_FLAGS() == 1

// overloaded template funtion that initializes the parameter list
// called m_params and the m_arity member.

#define LUABIND_INIT_PARAM(z, n, _) m_params_.push_back(LUABIND_TYPEID(A##n));
#define LUABIND_POLICY_DECL(z,n,text) typedef typename find_conversion_policy<n + 1, Policies>::type BOOST_PP_CAT(p,n);
#define LUABIND_ARITY(z,n,text) + BOOST_PP_CAT(p,n)::has_arg

template<class R BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A), class Policies>
overload_rep(R(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), Policies*)
	: fun(reinterpret_cast<void(*)()>(f))
{
	m_params_.reserve(BOOST_PP_ITERATION());
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_INIT_PARAM, _)
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POLICY_DECL, _)

	m_arity = 0 BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_ARITY, _);
}

#undef LUABIND_INIT_PARAM
#undef LUABIND_POLICY_DECL
#undef LUABIND_ARITY

#elif BOOST_PP_ITERATION_FLAGS() == 2

template<class Policies BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
static int call(T(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies*)
{
	int nargs = lua_gettop(L);

	typedef typename find_conversion_policy<0, Policies>::type converter_policy_ret;
	typename mpl::apply_wrap2<converter_policy_ret,T,cpp_to_lua>::type converter_ret;

	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
		converter_ret.apply(L, f
				(
				 BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, _)
				));
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, _)

	int nret = lua_gettop(L) - nargs;

	const int indices[] =
	{
		-1		/* self */,
		nargs + nret	/* result*/
		BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, _)
	};

	policy_list_postcall<Policies>::apply(L, indices);

	return maybe_yield<Policies>::apply(L, nret);
}



#elif BOOST_PP_ITERATION_FLAGS() == 3

template<class Policies BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
static int call(void(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies*)
{
	int nargs = lua_gettop(L);

	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
		f
		(
		 BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, _)
		);
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, _)

	int nret = lua_gettop(L) - nargs;

	const int indices[] =
	{
		-1		/* self */,
		nargs + nret	/* result*/
		BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, _)
	};

	policy_list_postcall<Policies>::apply(L, indices);

	return maybe_yield<Policies>::apply(L, nret);
}


#elif BOOST_PP_ITERATION_FLAGS() == 4

template<class Policies, class R BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
int call(R(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies* policies)
{
	return free_functions::returns<R>::call(f, L, policies);
}

#elif BOOST_PP_ITERATION_FLAGS() == 5

	template<class Policies, class R BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
	static int match(R(*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies* policies)
	{
		//if (lua_gettop(L) != BOOST_PP_ITERATION()) return -1;
		typedef constructor<BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)> ParameterTypes;
		return match_params(L, 1, static_cast<ParameterTypes*>(0), policies);
	}



#endif

