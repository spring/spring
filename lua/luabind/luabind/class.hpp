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


#ifndef LUABIND_CLASS_HPP_INCLUDED
#define LUABIND_CLASS_HPP_INCLUDED

/*
	ISSUES:
	------------------------------------------------------

	* solved for member functions, not application operator *
	if we have a base class that defines a function a derived class must be able to
	override that function (not just overload). Right now we just add the other overload
	to the overloads list and will probably get an ambiguity. If we want to support this
	each method_rep must include a vector of type_info pointers for each parameter.
	Operators do not have this problem, since operators always have to have
	it's own type as one of the arguments, no ambiguity can occur. Application
	operator, on the other hand, would have this problem.
	Properties cannot be overloaded, so they should always be overridden.
	If this is to work for application operator, we really need to specify if an application
	operator is const or not.

	If one class registers two functions with the same name and the same
	signature, there's currently no error. The last registered function will
	be the one that's used.
	How do we know which class registered the function? If the function was
	defined by the base class, it is a legal operation, to override it.
	we cannot look at the pointer offset, since it always will be zero for one of the bases.



	TODO:
	------------------------------------------------------

 	finish smart pointer support
		* the adopt policy should not be able to adopt pointers to held_types. This
		must be prohibited.
		* name_of_type must recognize holder_types and not return "custom"

	document custom policies, custom converters

	store the instance object for policies.

	support the __concat metamethod. This is a bit tricky, since it cannot be
	treated as a normal operator. It is a binary operator but we want to use the
	__tostring implementation for both arguments.
	
*/

#include <luabind/prefix.hpp>
#include <luabind/config.hpp>

#include <string>
#include <map>
#include <vector>
#include <cassert>

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/logical.hpp>

#include <luabind/config.hpp>
#include <luabind/scope.hpp>
#include <luabind/raw_policy.hpp>
#include <luabind/back_reference.hpp>
#include <luabind/detail/constructor.hpp>
#include <luabind/detail/call.hpp>
#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/primitives.hpp>
#include <luabind/detail/property.hpp>
#include <luabind/detail/typetraits.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/method_rep.hpp>
#include <luabind/detail/construct_rep.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/calc_arity.hpp>
#include <luabind/detail/call_member.hpp>
#include <luabind/detail/enum_maker.hpp>
#include <luabind/detail/get_signature.hpp>
#include <luabind/detail/implicit_cast.hpp>
#include <luabind/detail/operator_id.hpp>
#include <luabind/detail/pointee_typeid.hpp>
#include <luabind/detail/link_compatibility.hpp>

// to remove the 'this' used in initialization list-warning
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4355)
#endif


namespace luabind
{	
	namespace detail
	{
		struct unspecified {};

		template<class Derived> struct operator_;

		struct you_need_to_define_a_get_const_holder_function_for_your_smart_ptr {};
	}

	template<class T, class X1 = detail::unspecified, class X2 = detail::unspecified, class X3 = detail::unspecified>
	struct class_;

	// TODO: this function will only be invoked if the user hasn't defined a correct overload
	// maybe we should have a static assert in here?
	inline detail::you_need_to_define_a_get_const_holder_function_for_your_smart_ptr*
	get_const_holder(...)
	{
		return 0;
	}

	namespace detail
	{
		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, class A)>
		double is_bases_helper(const bases<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, A)>&);

#ifndef BOOST_MSVC
		template<class T>
		char is_bases_helper(const T&);
#else
		char is_bases_helper(...);
#endif

		template<class T>
		struct is_bases
		{
			static const T& t;

			BOOST_STATIC_CONSTANT(bool, value = sizeof(is_bases_helper(t)) == sizeof(double));
			typedef boost::mpl::bool_<value> type;
			BOOST_MPL_AUX_LAMBDA_SUPPORT(1,is_bases,(T))
		};

		double is_not_unspecified_helper(const unspecified*);
		char is_not_unspecified_helper(...);

		template<class T>
		struct is_not_unspecified
		{
			BOOST_STATIC_CONSTANT(bool, value = sizeof(is_not_unspecified_helper(static_cast<T*>(0))) == sizeof(char));
			typedef boost::mpl::bool_<value> type;
			BOOST_MPL_AUX_LAMBDA_SUPPORT(1,is_not_unspecified,(T))
		};

		template<class Predicate>
		struct get_predicate
		{
			typedef typename boost::mpl::and_<
			  	is_not_unspecified<boost::mpl::_1>
			  , Predicate
			> type;
		};

		template<class Parameters, class Predicate, class DefaultValue>
		struct extract_parameter
		{
			typedef typename get_predicate<Predicate>::type pred;
			typedef typename boost::mpl::find_if<Parameters, pred>::type iterator;
			typedef typename boost::mpl::eval_if<
				boost::is_same<
					iterator
				  , typename boost::mpl::end<Parameters>::type
				>
			  , boost::mpl::identity<DefaultValue>
			  , boost::mpl::deref<iterator>
			>::type type;
		};

		template<class Fn, class Class, class Policies>
		struct mem_fn_callback
		{
			typedef int result_type;

			int operator()(lua_State* L) const
			{
				return call(fn, (Class*)0, L, (Policies*)0);
			}

			mem_fn_callback(Fn fn_)
				: fn(fn_)
			{
			}

			Fn fn;
		};

		template<class Fn, class Class, class Policies>
		struct mem_fn_matcher
		{
			typedef int result_type;

			int operator()(lua_State* L) const
			{
				return match(fn, L, (Class*)0, (Policies*)0);
			}

			mem_fn_matcher(Fn fn_)
				: fn(fn_)
			{
			}

			Fn fn;
		};

		struct pure_virtual_tag
		{
			static void precall(lua_State*, index_map const&) {}
			static void postcall(lua_State*, index_map const&) {}
		};

		template<class Policies>
		struct has_pure_virtual
		{
			typedef typename boost::mpl::eval_if<
				boost::is_same<pure_virtual_tag, typename Policies::head>
			  , boost::mpl::true_
			  , has_pure_virtual<typename Policies::tail>
			>::type type;

			BOOST_STATIC_CONSTANT(bool, value = type::value);
		};

		template<>
		struct has_pure_virtual<null_type>
		{
			BOOST_STATIC_CONSTANT(bool, value = false);
			typedef boost::mpl::bool_<value> type;
		};

		// prints the types of the values on the stack, in the
		// range [start_index, lua_gettop()]

		LUABIND_API std::string stack_content_by_name(lua_State* L, int start_index);
	
		struct LUABIND_API create_class
		{
			static int stage1(lua_State* L);
			static int stage2(lua_State* L);
		};

		// if the class is held by a smart pointer, we need to be able to
		// implicitly dereference the pointer when needed.

		template<class UnderlyingT, class HeldT>
		struct extract_underlying_type
		{
			static void* extract(void* ptr)
			{
				HeldT& held_obj = *reinterpret_cast<HeldT*>(ptr);
				UnderlyingT* underlying_ptr = static_cast<UnderlyingT*>(get_pointer(held_obj));
				return underlying_ptr;
			}
		};

		template<class UnderlyingT, class HeldT>
		struct extract_underlying_const_type
		{
			static const void* extract(void* ptr)
			{
				HeldT& held_obj = *reinterpret_cast<HeldT*>(ptr);
				const UnderlyingT* underlying_ptr = static_cast<const UnderlyingT*>(get_pointer(held_obj));
				return underlying_ptr;
			}
		};

		template<class HeldType>
		struct internal_holder_extractor
		{
			typedef void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return &detail::extract_underlying_type<T, HeldType>::extract;
			}
		};

		template<>
		struct internal_holder_extractor<detail::null_type>
		{
			typedef void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return 0;
			}
		};


		template<class HeldType, class ConstHolderType>
		struct convert_holder
		{
			static void apply(void* holder, void* target)
			{
				new(target) ConstHolderType(*reinterpret_cast<HeldType*>(holder));
			};
		};


		template<class HeldType>
		struct const_converter
		{
			typedef void(*converter_fun)(void*, void*);

			template<class ConstHolderType>
			static converter_fun apply(ConstHolderType*)
			{
				return &detail::convert_holder<HeldType, ConstHolderType>::apply;
			}
		};

		template<>
		struct const_converter<detail::null_type>
		{
			typedef void(*converter_fun)(void*, void*);

			template<class T>
			static converter_fun apply(T*)
			{
				return 0;
			}
		};




		template<class HeldType>
		struct internal_const_holder_extractor
		{
			typedef const void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return get_extractor(detail::type_<T>(), get_const_holder(static_cast<HeldType*>(0)));
			}
		private:
			template<class T, class ConstHolderType>
			static extractor_fun get_extractor(detail::type_<T>, ConstHolderType*)
			{
				return &detail::extract_underlying_const_type<T, ConstHolderType>::extract;
			}
		};

		template<>
		struct internal_const_holder_extractor<detail::null_type>
		{
			typedef const void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return 0;
			}
		};



		// this is simply a selector that returns the type_info
		// of the held type, or invalid_type_info if we don't have
		// a held_type
		template<class HeldType>
		struct internal_holder_type
		{
			static LUABIND_TYPE_INFO apply()
			{
				return LUABIND_TYPEID(HeldType);
			}
		};

		template<>
		struct internal_holder_type<detail::null_type>
		{
			static LUABIND_TYPE_INFO apply()
			{
				return LUABIND_INVALID_TYPE_INFO;
			}
		};


		// this is the actual held_type constructor
		template<class HeldType, class T>
		struct internal_construct_holder
		{
			static void apply(void* target, void* raw_pointer)
			{
				new(target) HeldType(static_cast<T*>(raw_pointer));
			}
		};

		// this is the actual held_type default constructor
		template<class HeldType, class T>
		struct internal_default_construct_holder
		{
			static void apply(void* target)
			{
				new(target) HeldType();
			}
		};

		// the following two functions are the ones that returns
		// a pointer to a held_type_constructor, or 0 if there
		// is no held_type
		template<class HeldType>
		struct holder_constructor
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return &internal_construct_holder<HeldType, T>::apply;
			}
		};

		template<>
		struct holder_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};

		// the following two functions are the ones that returns
		// a pointer to a const_held_type_constructor, or 0 if there
		// is no held_type
		template<class HolderType>
		struct const_holder_constructor
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return get_const_holder_constructor(detail::type_<T>(), get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class T, class ConstHolderType>
			static constructor get_const_holder_constructor(detail::type_<T>, ConstHolderType*)
			{
				return &internal_construct_holder<ConstHolderType, T>::apply;
			}
		};

		template<>
		struct const_holder_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};


		// the following two functions are the ones that returns
		// a pointer to a held_type_constructor, or 0 if there
		// is no held_type. The holder_type is default constructed
		template<class HeldType>
		struct holder_default_constructor
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return &internal_default_construct_holder<HeldType, T>::apply;
			}
		};

		template<>
		struct holder_default_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};


		// the following two functions are the ones that returns
		// a pointer to a const_held_type_constructor, or 0 if there
		// is no held_type. The constructed held_type is default
		// constructed
		template<class HolderType>
		struct const_holder_default_constructor
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return get_const_holder_default_constructor(detail::type_<T>(), get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class T, class ConstHolderType>
			static constructor get_const_holder_default_constructor(detail::type_<T>, ConstHolderType*)
			{
				return &internal_default_construct_holder<ConstHolderType, T>::apply;
			}
		};

		template<>
		struct const_holder_default_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};




		// this is a selector that returns the size of the held_type
		// or 0 if we don't have a held_type
		template <class HolderType>
		struct internal_holder_size
		{
			static int apply() { return get_internal_holder_size(get_const_holder(static_cast<HolderType*>(0))); }
		private:
			template<class ConstHolderType>
			static int get_internal_holder_size(ConstHolderType*)
			{
				return max_c<sizeof(HolderType), sizeof(ConstHolderType)>::value;
			}
		};

		template <>
		struct internal_holder_size<detail::null_type>
		{
			static int apply() {	return 0; }
		};


		// if we have a held type, return the destructor to it
		// note the difference. The held_type should only be destructed (not deleted)
		// since it's constructed in the lua userdata
		template<class HeldType>
		struct internal_holder_destructor
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return &detail::destruct_only_s<HeldType>::apply;
			}
		};

		// if we don't have a held type, return the destructor of the raw type
		template<>
		struct internal_holder_destructor<detail::null_type>
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return &detail::delete_s<T>::apply;
			}
		};

		
		// if we have a held type, return the destructor to it's const version
		template<class HolderType>
		struct internal_const_holder_destructor
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return const_holder_type_destructor(get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class ConstHolderType>
			static destructor_t const_holder_type_destructor(ConstHolderType*)
			{
				return &detail::destruct_only_s<ConstHolderType>::apply;
			}

		};

		// if we don't have a held type, return the destructor of the raw type
		template<>
		struct internal_const_holder_destructor<detail::null_type>
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return 0;
			}
		};




		template<class HolderType>
		struct get_holder_alignment
		{
			static int apply()
			{
				return internal_alignment(get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class ConstHolderType>
			static int internal_alignment(ConstHolderType*)
			{
				return detail::max_c<boost::alignment_of<HolderType>::value
					, boost::alignment_of<ConstHolderType>::value>::value;
			}
		};

		template<>
		struct get_holder_alignment<detail::null_type>
		{
			static int apply()
			{
				return 1;
			}
		};


	} // detail

	namespace detail {

		template<class T>
		struct static_scope
		{
			static_scope(T& self_) : self(self_)
			{
			}

			T& operator[](scope s) const
			{
				self.add_inner_scope(s);
				return self;
			}

		private:
			template<class U> void operator,(U const&) const;
			void operator=(static_scope const&);
			
			T& self;
		};

		struct class_registration;

		struct LUABIND_API class_base : scope
		{
		public:
			class_base(char const* name);		

			struct base_desc
			{
				LUABIND_TYPE_INFO type;
				int ptr_offset;
			};

			void init(
				LUABIND_TYPE_INFO type
				, LUABIND_TYPE_INFO holder_type
				, LUABIND_TYPE_INFO const_holder_type
				, void*(*extractor)(void*)
				, const void*(*const_extractor)(void*)
				, void(*const_converter)(void*,void*)
				, void(*holder_constructor)(void*,void*)
				, void(*const_holder_constructor)(void*,void*)
				, void(*holder_default_constructor)(void*)
				, void(*const_holder_default_constructor)(void*)
				, void(*destructor)(void*)
				, void(*const_holder_destructor)(void*)
				, void(*m_adopt_fun)(void*)
				, int holder_size
				, int holder_alignment);

			void add_getter(
				const char* name
				, const boost::function2<int, lua_State*, int>& g);

#ifdef LUABIND_NO_ERROR_CHECKING
			void add_setter(
				const char* name
				, const boost::function2<int, lua_State*, int>& s);
#else
			void add_setter(
				const char* name
				, const boost::function2<int, lua_State*, int>& s
				, int (*match)(lua_State*, int)
				, void (*get_sig_ptr)(lua_State*, std::string&));
#endif

			void add_base(const base_desc& b);
			void add_constructor(const detail::construct_rep::overload_t& o);	
			void add_method(const char* name, const detail::overload_rep& o);

#ifndef LUABIND_NO_ERROR_CHECKING
			void add_operator(
				int op_id
				,  int(*func)(lua_State*)
				, int(*matcher)(lua_State*)
				, void(*sig)(lua_State*
				, std::string&)
				, int arity);
#else
			void add_operator(
				int op_id
				,  int(*func)(lua_State*)
				, int(*matcher)(lua_State*)
				, int arity);
#endif

			const char* name() const;

			void add_static_constant(const char* name, int val);
			void add_inner_scope(scope& s);

		private:
			class_registration* m_registration;
		};
	
        template<class T, class W>
        struct adopt_function
		{
		    static void execute(void* p)
            {
			    wrapped_self_t& self = wrap_access::ref(
					*static_cast<W*>(static_cast<T*>(p))
				);

				LUABIND_CHECK_STACK(self.state());

				self.get(self.state());
				self.m_strong_ref.set(self.state());
            }
        };

	} // namespace detail

	// registers a class in the lua environment
	template<class T, class X1, class X2, class X3>
	struct class_: detail::class_base 
	{
		typedef class_<T, X1, X2, X3> self_t;

	private:

		template<class A, class B, class C, class D>
		class_(const class_<A,B,C,D>&);

	public:

		// WrappedType MUST inherit from T
		typedef typename detail::extract_parameter<
		    boost::mpl::vector3<X1,X2,X3>
		  , boost::is_base_and_derived<T, boost::mpl::_>
		  , detail::null_type
		>::type WrappedType;

		typedef typename detail::extract_parameter<
		    boost::mpl::list3<X1,X2,X3>
		  , boost::mpl::not_<
		        boost::mpl::or_<
				    boost::mpl::or_<
					    detail::is_bases<boost::mpl::_>
					  , boost::is_base_and_derived<boost::mpl::_, T>
					>
				  , boost::is_base_and_derived<T, boost::mpl::_>
				>
			>
		  , detail::null_type
		>::type HeldType;

		// this function generates conversion information
		// in the given class_rep structure. It will be able
		// to implicitly cast to the given template type
		template<class To>
		void gen_base_info(detail::type_<To>)
		{
			// fist, make sure the given base class is registered.
			// if it's not registered we can't push it's lua table onto
			// the stack because it doesn't have a table

			// try to cast this type to the base type and remember
			// the pointer offset. For multiple inheritance the pointer
			// may change when casting. Since we need to be able to
			// cast we need this pointer offset.
			// store the information in this class' base class-vector
			base_desc base;
			base.type = LUABIND_TYPEID(To);
			base.ptr_offset = detail::ptr_offset(detail::type_<T>(), detail::type_<To>());
			add_base(base);
		}

		void gen_base_info(detail::type_<detail::null_type>)
		{}

#define LUABIND_GEN_BASE_INFO(z, n, text) gen_base_info(detail::type_<B##n>());

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, class B)>
		void generate_baseclass_list(detail::type_<bases<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, B)> >)
		{
			BOOST_PP_REPEAT(LUABIND_MAX_BASES, LUABIND_GEN_BASE_INFO, _)
		}

#undef LUABIND_GEN_BASE_INFO

		class_(const char* name): class_base(name), scope(*this)
		{
#ifndef NDEBUG
			detail::check_link_compatibility();
#endif
		   	init(); 
		}

		template<class F>
		class_& def(const char* name, F f)
		{
			return this->virtual_def(
				name, f, detail::null_type()
			  , detail::null_type(), boost::mpl::true_());
		}

		// virtual functions
		template<class F, class DefaultOrPolicies>
		class_& def(char const* name, F fn, DefaultOrPolicies default_or_policies)
		{
			return this->virtual_def(
				name, fn, default_or_policies, detail::null_type()
			  , LUABIND_MSVC_TYPENAME detail::is_policy_cons<DefaultOrPolicies>::type());
		}

		template<class F, class Default, class Policies>
		class_& def(char const* name, F fn
			, Default default_, Policies const& policies)
		{
			return this->virtual_def(
				name, fn, default_
			  , policies, boost::mpl::false_());
		}

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
		class_& def(constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, A)> sig)
		{
            return this->def_constructor(
				boost::is_same<WrappedType, detail::null_type>()
			  , &sig
			  , detail::null_type()
			);
		}

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A), class Policies>
		class_& def(constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, A)> sig, const Policies& policies)
		{
            return this->def_constructor(
				boost::is_same<WrappedType, detail::null_type>()
			  , &sig
			  , policies
			);
		}

		template<class Getter>
		class_& property(const char* name, Getter g)
		{
			add_getter(name, boost::bind<int>(detail::get_caller<T, Getter, detail::null_type>(), _1, _2, g));
			return *this;
		}

		template<class Getter, class MaybeSetter>
		class_& property(const char* name, Getter g, MaybeSetter s)
		{
			return property_impl(name, g, s, boost::mpl::bool_<detail::is_policy_cons<MaybeSetter>::value>());
		}

		template<class Getter, class Setter, class GetPolicies>
		class_& property(const char* name, Getter g, Setter s, const GetPolicies& get_policies)
		{
			add_getter(name, boost::bind<int>(detail::get_caller<T, Getter, GetPolicies>(get_policies), _1, _2, g));
#ifndef LUABIND_NO_ERROR_CHECKING
			add_setter(
				name
				, boost::bind<int>(detail::set_caller<T, Setter, detail::null_type>(), _1, _2, s)
				, detail::gen_set_matcher((Setter)0, (detail::null_type*)0)
				, &detail::get_member_signature<Setter>::apply);
#else
			add_setter(
				name
				, boost::bind<int>(detail::set_caller<T, Setter, detail::null_type>(), _1, _2, s));
#endif
			return *this;
		}

		template<class Getter, class Setter, class GetPolicies, class SetPolicies>
		class_& property(const char* name
									, Getter g, Setter s
									, const GetPolicies& get_policies
									, const SetPolicies& set_policies)
		{
			add_getter(name, boost::bind<int>(detail::get_caller<T, Getter, GetPolicies>(get_policies), _1, _2, g));
#ifndef LUABIND_NO_ERROR_CHECKING
			add_setter(
				name
				, boost::bind<int>(detail::set_caller<T, Setter, SetPolicies>(), _1, _2, s)
				, detail::gen_set_matcher((Setter)0, (SetPolicies*)0)
				, &detail::get_member_signature<Setter>::apply);
#else
			add_setter(name, boost::bind<int>(detail::set_caller<T, Setter, SetPolicies>(set_policies), _1, _2, s));
#endif
			return *this;
		}

		template<class D>
		class_& def_readonly(const char* name, D T::*member_ptr)
		{
			add_getter(name, boost::bind<int>(detail::auto_get<T,D,detail::null_type>(), _1, _2, member_ptr));
			return *this;
		}

		template<class D, class Policies>
		class_& def_readonly(const char* name, D T::*member_ptr, const Policies& policies)
		{
			add_getter(name, boost::bind<int>(detail::auto_get<T,D,Policies>(policies), _1, _2, member_ptr));
			return *this;
		}

		template<class D>
		class_& def_readwrite(const char* name, D T::*member_ptr)
		{
			add_getter(name, boost::bind<int>(detail::auto_get<T,D,detail::null_type>(), _1, _2, member_ptr));
#ifndef LUABIND_NO_ERROR_CHECKING
			add_setter(
				name
				, boost::bind<int>(detail::auto_set<T,D,detail::null_type>(), _1, _2, member_ptr)
				, &detail::set_matcher<D, detail::null_type>::apply
				, &detail::get_setter_signature<D>::apply);
#else
			add_setter(name, boost::bind<int>(detail::auto_set<T,D,detail::null_type>(), _1, _2, member_ptr));
#endif
			return *this;
		}

		template<class D, class GetPolicies>
		class_& def_readwrite(const char* name, D T::*member_ptr, const GetPolicies& get_policies)
		{
			add_getter(name, boost::bind<int>(detail::auto_get<T,D,GetPolicies>(get_policies), _1, _2, member_ptr));
#ifndef LUABIND_NO_ERROR_CHECKING
			add_setter(
				name
				, boost::bind<int>(detail::auto_set<T,D,detail::null_type>(), _1, _2, member_ptr)
				, &detail::set_matcher<D, detail::null_type>::apply
				, &detail::get_setter_signature<D>::apply);
#else
			add_setter(name, boost::bind<int>(detail::auto_set<T,D,detail::null_type>(), _1, _2, member_ptr));
#endif
			return *this;
		}

		template<class D, class GetPolicies, class SetPolicies>
		class_& def_readwrite(const char* name, D T::*member_ptr, const GetPolicies& get_policies, const SetPolicies& set_policies)
		{
			add_getter(name, boost::bind<int>(detail::auto_get<T,D,GetPolicies>(get_policies), _1, _2, member_ptr));
#ifndef LUABIND_NO_ERROR_CHECKING
			add_setter(
				name
				, boost::bind<int>(detail::auto_set<T,D,SetPolicies>(), _1, _2, member_ptr)
				, &detail::set_matcher<D, SetPolicies>::apply
				, &detail::get_setter_signature<D>::apply);
#else
			add_setter(name, boost::bind<int>(detail::auto_set<T,D,SetPolicies>(set_policies), _1, _2, member_ptr));
#endif
			return *this;
		}

		template<class Derived, class Policies>
		class_& def(detail::operator_<Derived>, Policies const& policies)
		{
			return this->def(
				Derived::name()
			  , &Derived::template apply<T, Policies>::execute
			  , raw(_1) + policies
			);
		}

		template<class Derived>
		class_& def(detail::operator_<Derived>)
		{
			return this->def(
				Derived::name()
			  , &Derived::template apply<T, detail::null_type>::execute
			  , raw(_1)
			);
		}

/*
		template<class op_id, class Left, class Right, class Policies>
		class_& def(detail::operator_<op_id, Left, Right>, const Policies& policies)
		{
			typedef typename detail::operator_unwrapper<Policies, op_id, T, Left, Right> op_type;
#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, &detail::get_signature<constructor<typename op_type::left_t, typename op_type::right_t> >::apply
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#else
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#endif
			return *this;
		}

		template<class op_id, class Left, class Right>
		class_& def(detail::operator_<op_id, Left, Right>)
		{
			typedef typename detail::operator_unwrapper<detail::null_type, op_id, T, Left, Right> op_type;

#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, &detail::get_signature<constructor<LUABIND_MSVC_TYPENAME op_type::left_t, LUABIND_MSVC_TYPENAME op_type::right_t> >::apply
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#else
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#endif
			return *this;
		}

		template<class Signature, bool Constant>
		class_& def(detail::application_operator<Signature, Constant>*)
		{
			typedef detail::application_operator<Signature, Constant, detail::null_type> op_t;

			int arity = detail::calc_arity<Signature::arity>::apply(
				Signature(), static_cast<detail::null_type*>(0));

#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, &detail::get_signature<Signature>::apply
				, arity + 1);
#else
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, arity + 1);
#endif

			return *this;
		}

		template<class Signature, bool Constant, class Policies>
		class_& def(detail::application_operator<Signature, Constant>*, const Policies& policies)
		{
			typedef detail::application_operator<Signature, Constant, Policies> op_t;

			int arity = detail::calc_arity<Signature::arity>::apply(Signature(), static_cast<Policies*>(0));

#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, &detail::get_signature<Signature>::apply
				, arity + 1);
#else
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, arity + 1);
#endif

			return *this;
		}
*/
		detail::enum_maker<self_t> enum_(const char*)
		{
			return detail::enum_maker<self_t>(*this);
		}
		
		detail::static_scope<self_t> scope;
		
	private:
		void operator=(class_ const&);

		void init()
		{
			typedef typename detail::extract_parameter<
					boost::mpl::list3<X1,X2,X3>
				,	boost::mpl::or_<
							detail::is_bases<boost::mpl::_>
						,	boost::is_base_and_derived<boost::mpl::_, T>
					>
				,	no_bases
			>::type bases_t;

			typedef typename 
				boost::mpl::if_<detail::is_bases<bases_t>
					,	bases_t
					,	bases<bases_t>
				>::type Base;
	
			class_base::init(LUABIND_TYPEID(T)
				, detail::internal_holder_type<HeldType>::apply()
				, detail::pointee_typeid(
					get_const_holder(static_cast<HeldType*>(0)))
				, detail::internal_holder_extractor<HeldType>::apply(detail::type_<T>())
				, detail::internal_const_holder_extractor<HeldType>::apply(detail::type_<T>())
				, detail::const_converter<HeldType>::apply(
					get_const_holder((HeldType*)0))
				, detail::holder_constructor<HeldType>::apply(detail::type_<T>())
				, detail::const_holder_constructor<HeldType>::apply(detail::type_<T>())
				, detail::holder_default_constructor<HeldType>::apply(detail::type_<T>())
				, detail::const_holder_default_constructor<HeldType>::apply(detail::type_<T>())
				, get_adopt_fun((WrappedType*)0) // adopt fun
				, detail::internal_holder_destructor<HeldType>::apply(detail::type_<T>())
				, detail::internal_const_holder_destructor<HeldType>::apply(detail::type_<T>())
				, detail::internal_holder_size<HeldType>::apply()
				, detail::get_holder_alignment<HeldType>::apply());

			generate_baseclass_list(detail::type_<Base>());
		}

		template<class Getter, class GetPolicies>
		class_& property_impl(const char* name,
									 Getter g,
									 GetPolicies policies,
									 boost::mpl::bool_<true>)
		{
			add_getter(name, boost::bind<int>(detail::get_caller<T,Getter,GetPolicies>(policies), _1, _2, g));
			return *this;
		}

		template<class Getter, class Setter>
		class_& property_impl(const char* name,
									 Getter g,
									 Setter s,
									 boost::mpl::bool_<false>)
		{
			add_getter(name, boost::bind<int>(detail::get_caller<T,Getter,detail::null_type>(), _1, _2, g));
#ifndef LUABIND_NO_ERROR_CHECKING
			add_setter(
				name
				, boost::bind<int>(detail::set_caller<T, Setter, detail::null_type>(), _1, _2, s)
				, detail::gen_set_matcher((Setter)0, (detail::null_type*)0)
				, &detail::get_member_signature<Setter>::apply);
#else
			add_setter(name, boost::bind<int>(detail::set_caller<T,Setter,detail::null_type>(), _1, _2, s));
#endif
			return *this;
		}

		// these handle default implementation of virtual functions
		template<class F, class Policies>
		class_& virtual_def(char const* name, F const& fn
			, Policies const&, detail::null_type, boost::mpl::true_)
		{
			// normal def() call
			detail::overload_rep o(fn, static_cast<Policies*>(0));

			o.set_match_fun(detail::mem_fn_matcher<F, T, Policies>(fn));
			o.set_fun(detail::mem_fn_callback<F, T, Policies>(fn));

#ifndef LUABIND_NO_ERROR_CHECKING
			o.set_sig_fun(&detail::get_member_signature<F>::apply);
#endif
			this->add_method(name, o);
			return *this;
		}

		template<class F, class Default, class Policies>
		class_& virtual_def(char const* name, F const& fn
			, Default const& default_, Policies const&, boost::mpl::false_)
		{
			// default_ is a default implementation
			// policies is either null_type or a policy list

			// normal def() call
			detail::overload_rep o(fn, (Policies*)0);

			o.set_match_fun(detail::mem_fn_matcher<F, T, Policies>(fn));
			o.set_fun(detail::mem_fn_callback<F, T, Policies>(fn));

			o.set_fun_static(
				detail::mem_fn_callback<Default, T, Policies>(default_));

#ifndef LUABIND_NO_ERROR_CHECKING
			o.set_sig_fun(&detail::get_member_signature<F>::apply);
#endif

			this->add_method(name, o);
			// register virtual function
			return *this;
		}

        template<class Signature, class Policies>
		class_& def_constructor(
			boost::mpl::true_ /* HasWrapper */
          , Signature*
          , Policies const&)
        {	
			detail::construct_rep::overload_t o;

			o.set_constructor(
				&detail::construct_class<
					T
				  , Policies
				  , Signature
				>::apply);

			o.set_match_fun(
				&detail::constructor_match<
				    Signature
			      , 2
			      , Policies
			    >::apply);

#ifndef LUABIND_NO_ERROR_CHECKING
			o.set_sig_fun(&detail::get_signature<Signature>::apply);
#endif
			o.set_arity(detail::calc_arity<Signature::arity>::apply(Signature(), (Policies*)0));
			this->add_constructor(o);
            return *this;
        }

        template<class Signature, class Policies>
		class_& def_constructor(
			boost::mpl::false_ /* !HasWrapper */
          , Signature*
          , Policies const&)
		{
			detail::construct_rep::overload_t o;

			o.set_constructor(
				&detail::construct_wrapped_class<
					T
				  , WrappedType
				  , Policies
				  , Signature
				>::apply);

			o.set_match_fun(
				&detail::constructor_match<
				    Signature
			      , 2
			      , Policies
			    >::apply);

#ifndef LUABIND_NO_ERROR_CHECKING
			o.set_sig_fun(&detail::get_signature<Signature>::apply);
#endif
			o.set_arity(detail::calc_arity<Signature::arity>::apply(Signature(), (Policies*)0));
			this->add_constructor(o);
            return *this;
        }

		typedef void(*adopt_fun_t)(void*);

		template<class W>
		adopt_fun_t get_adopt_fun(W*)
		{
            return &detail::adopt_function<T, W>::execute;
		}

		adopt_fun_t get_adopt_fun(detail::null_type*)
		{
			return 0;
		}
	};

	namespace 
	{
		LUABIND_ANONYMOUS_FIX detail::policy_cons<
			detail::pure_virtual_tag
		  , detail::null_type
		> pure_virtual;
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // LUABIND_CLASS_HPP_INCLUDED

