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

#ifndef LUABIND_OBJECT_HPP_INCLUDED
#define LUABIND_OBJECT_HPP_INCLUDED

#include <iterator>

#include <luabind/prefix.hpp>
#include <luabind/config.hpp>
#include <luabind/error.hpp>
#include <luabind/detail/pcall.hpp>
#include <luabind/detail/stack_utils.hpp>

#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/tuple/tuple.hpp>

namespace luabind
{
	class object;

	namespace detail
	{
		class proxy_object;
		class proxy_raw_object;
		class proxy_array_object;

		template<class T>
		void convert_to_lua(lua_State*, const T&);

		template<int Index, class T, class Policies>
		void convert_to_lua_p(lua_State*, const T&, const Policies&);

		template<int Index>
		struct push_args_from_tuple
		{
			template<class H, class T, class Policies>
			inline static void apply(lua_State* L, const boost::tuples::cons<H, T>& x, const Policies& p) 
			{
				convert_to_lua_p<Index>(L, *x.get_head(), p);
				push_args_from_tuple<Index+1>::apply(L, x.get_tail(), p);
			}

			template<class H, class T>
			inline static void apply(lua_State* L, const boost::tuples::cons<H, T>& x) 
			{
				convert_to_lua(L, *x.get_head());
				push_args_from_tuple<Index+1>::apply(L, x.get_tail());
			}

			template<class Policies>
			inline static void apply(lua_State*, const boost::tuples::null_type&, const Policies&) {};

			inline static void apply(lua_State*, const boost::tuples::null_type&) {};

		};

		template<class Tuple>
		class proxy_caller
		{
		friend class luabind::object;
		public:

			proxy_caller(luabind::object* o, const Tuple args)
				: m_obj(o)
				, m_args(args)
				, m_called(false)
			{
			}

			proxy_caller(const detail::proxy_caller<Tuple>& rhs)
				: m_obj(rhs.m_obj)
				, m_args(rhs.m_args)
				, m_called(rhs.m_called)
			{
				rhs.m_called = true;
			}

			~proxy_caller();
			operator luabind::object();

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
	#define LUABIND_SEMICOLON
#else
	#define LUABIND_SEMICOLON ;
#endif

			template<class Policies>
			luabind::object operator[](const Policies& p) LUABIND_SEMICOLON
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			{
				m_called = true;
				lua_State* L = m_obj->lua_state();
				m_obj->pushvalue();
				detail::push_args_from_tuple<1>::apply(L, m_args, p);
				if (pcall(L, boost::tuples::length<Tuple>::value, 1))
				{ 
#ifndef LUABIND_NO_EXCEPTIONS
					throw error(L);
#else
					error_callback_fun e = get_error_callback();
					if (e) e(L);
	
					assert(0 && "the lua function threw an error and exceptions are disabled."
						"if you want to handle this error use luabind::set_error_callback()");
					std::terminate();
#endif
				}
				detail::lua_reference ref;
				ref.set(L);
				return luabind::object(m_obj->lua_state(), ref, true/*luabind::object::reference()*/);
			}
#endif


#undef LUABIND_SEMICOLON
		private:

			luabind::object* m_obj;
			Tuple m_args;
			mutable bool m_called;

		};


		class LUABIND_API proxy_object
		{
		friend class luabind::object;
		friend class luabind::detail::proxy_array_object;
		friend class luabind::detail::proxy_raw_object;
//		template<class T> friend T object_cast(const proxy_object& obj);
		public:

			template<class T>
			proxy_object& operator=(const T& val)
			{
				//std::cout << "proxy assigment\n";
				lua_State* L = m_obj->m_state;
				m_obj->pushvalue();
				m_key.get(L);
				detail::convert_to_lua(L, val);
				lua_settable(L, -3);
				// pop table
				lua_pop(L, 1);
				return *this;
			}

			template<class T, class Policies>
			void assign(const T& val, const Policies& p)
			{
				//std::cout << "proxy assigment\n";
				lua_State* L = m_obj->m_state;
				m_obj->pushvalue();
				detail::getref(L, m_key_ref);
				detail::convert_to_lua_p(L, val, p);
				lua_settable(L, -3);
				// pop table
				lua_pop(L, 1);
				return *this;
			}

			template<class T>
			detail::proxy_object operator[](const T& key) const;

			proxy_object& operator=(const object& p);
			proxy_object& operator=(const proxy_object& p);
			proxy_object& operator=(const proxy_raw_object& p);
			proxy_object& operator=(const proxy_array_object& p);

			void swap(const proxy_object& rhs);
			proxy_object* operator->()
			{ return this; }

			operator luabind::object();

			int type() const
			{
				pushvalue();
				detail::stack_pop p(lua_state(), 1);
				return lua_type(lua_state(), -1);
			}

#define LUABIND_PROXY_RAW_AT_BODY											\
			{																										\
				lua_State* L = lua_state();\
				pushvalue();\
				detail::convert_to_lua(L, key);\
				lua_rawget(L, -2);\
				detail::lua_reference ref;\
				ref.set(L);\
				lua_pop(L, 1);\
				return object(L, ref, true);\
			}

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			template<class T>					
			inline object raw_at(const T& key)
			LUABIND_PROXY_RAW_AT_BODY
#else
			template<class T>
			inline object raw_at(const T& key);
#endif

#define LUABIND_PROXY_AT_BODY \
			{\
				lua_State* L = lua_state();\
				pushvalue();\
				detail::convert_to_lua(L, key);\
				lua_gettable(L, -2);\
				detail::lua_reference ref;\
				ref.set(L);\
				lua_pop(L, 1);\
				return object(L, ref, true);\
			}

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			template<class T>					
			inline object at(const T& key)
			LUABIND_PROXY_AT_BODY
#else
			template<class T>
			inline object at(const T& key);
#endif

			inline bool is_valid() const { return true; }
			lua_State* lua_state() const;
			void pushvalue() const;
			void set() const;

			// this is a safe substitute for an implicit converter to bool
			typedef void (proxy_object::*member_ptr)() const;
			operator member_ptr() const
			{
				if (is_valid()) return &proxy_object::dummy;
				return 0;
			}

		private:

			void dummy() const {}

			proxy_object(luabind::object* o, const lua_reference& key)
				: m_obj(o)
				, m_key(key)
			{
			}

			luabind::object* m_obj;
			detail::lua_reference m_key;
		};



		class LUABIND_API proxy_raw_object
		{
		friend class luabind::object;
		friend class luabind::detail::proxy_array_object;
		friend class luabind::detail::proxy_object;
//		template<class T> friend T luabind::object_cast(const proxy_object& obj);
		public:

			template<class T>
			proxy_raw_object& operator=(const T& val)
			{
				//std::cout << "proxy assigment\n";
				lua_State* L = m_obj->m_state;
				m_obj->pushvalue();
				detail::getref(L, m_key_ref);
				detail::convert_to_lua(L, val);
				lua_rawset(L, -3);
				// pop table
				lua_pop(L, 1);
				return *this;
			}

			template<class T, class Policies>
			void assign(const T& val, const Policies& p)
			{
				//std::cout << "proxy assigment\n";
				lua_State* L = m_obj->m_state;
				m_obj->pushvalue();
				detail::getref(L, m_key_ref);
				detail::convert_to_lua_p(L, val, p);
				lua_settable(L, -3);
				// pop table
				lua_pop(L, 1);
				return *this;
			}

			proxy_raw_object& operator=(const object& p);
			proxy_raw_object& operator=(const proxy_object& p);
			proxy_raw_object& operator=(const proxy_raw_object& p);
			proxy_raw_object& operator=(const proxy_array_object& p);
			void swap(const proxy_raw_object& rhs);
			proxy_raw_object* operator->()
			{ return this; }

			operator luabind::object();

			int type() const
			{
				pushvalue();
				detail::stack_pop p(lua_state(), 1);
				return lua_type(lua_state(), -1);
			}

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			template<class T>	
			inline object raw_at(const T& key)
			LUABIND_PROXY_RAW_AT_BODY
#else
			template<class T>
			inline object raw_at(const T& key);
#endif

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			template<class T>	
			inline object at(const T& key)
			LUABIND_PROXY_AT_BODY
#else
			template<class T>
			inline object at(const T& key);
#endif

			inline bool is_valid() const { return true; }
			lua_State* lua_state() const;
			void pushvalue() const;
			void set() const;

			// this is a safe substitute for an implicit converter to bool
			typedef void (proxy_raw_object::*member_ptr)() const;
			operator member_ptr() const
			{
				if (is_valid()) return &proxy_raw_object::dummy;
				return 0;
			}


		private:

			void dummy() const {}

			proxy_raw_object(luabind::object* o, const lua_reference& key)
				: m_obj(o)
				, m_key(key)
			{
			}

			luabind::object* m_obj;
			detail::lua_reference m_key;
		};



		class LUABIND_API proxy_array_object
		{
		friend class luabind::object;
		friend class luabind::detail::proxy_object;
		friend class luabind::detail::proxy_raw_object;
//		template<class T> friend T object_cast(const proxy_array_object& obj);
		public:

			template<class T>
			proxy_array_object& operator=(const T& val)
			{
				//std::cout << "array proxy assigment\n";
				lua_State* L = m_obj->m_state;
				m_obj->pushvalue();
				detail::convert_to_lua(L, val);
				lua_rawseti(L, -2, m_key);

				// pops the table
				lua_pop(L, 1);
				return *this;
			}

			template<class T, class Policies>
			void assign(const T& val, const Policies& p)
			{
				//std::cout << "proxy assigment\n";
				lua_State* L = m_obj->m_state;
				m_obj->pushvalue();
				detail::convert_to_lua_p(L, val, p);
				lua_rawseti(L, -2, m_key);
				// pop table
				lua_pop(L, 1);
				return *this;
			}

			proxy_array_object& operator=(const object& p);
			proxy_array_object& operator=(const proxy_object& p);
			proxy_array_object& operator=(const proxy_raw_object& p);
			proxy_array_object& operator=(const proxy_array_object& p);
			void swap(const proxy_array_object& rhs);

			proxy_array_object* operator->()
			{ return this; }
			operator luabind::object();

			int type() const
			{
				pushvalue();
				detail::stack_pop p(lua_state(), 1);
				return lua_type(lua_state(), -1);
			}

#define LUABIND_PROXY_ARRAY_RAW_AT_BODY\
			{\
				pushvalue();\
				detail::convert_to_lua(m_state, key);\
				lua_rawget(m_state, -2);\
				lua_reference ref;\
				ref.set(m_state);\
				lua_pop(m_state, 1);\
				return object(m_state, ref, true);\
			}

#define LUABIND_PROXY_ARRAY_AT_BODY\
			{\
				pushvalue();\
				detail::convert_to_lua(m_state, key);\
				lua_gettable(m_state, -2);\
				lua_reference ref;\
				ref.set(m_state);\
				lua_pop(m_state, 1);\
				return object(m_state, ref, true);\
			}

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			template<class T>	
			inline object at(const T& key)
			LUABIND_PROXY_ARRAY_AT_BODY
#else
			template<class T>
			inline object at(const T& key);
#endif


#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
			template<class T>	
			inline object raw_at(const T& key)
			LUABIND_PROXY_ARRAY_RAW_AT_BODY
#else
			template<class T>
			inline object raw_at(const T& key);
#endif

			inline bool is_valid() const { return true; }
			lua_State* lua_state() const;
			void pushvalue() const;
			void set() const;

			// this is a safe substitute for an implicit converter to bool
			typedef void (proxy_array_object::*member_ptr)() const;
			operator member_ptr() const
			{
				if (is_valid()) return &proxy_array_object::dummy;
				return 0;
			}

		private:

			void dummy() const {}

			proxy_array_object(luabind::object* o, int key)
				: m_obj(o)
				, m_key(key)
			{}
			luabind::object* m_obj;
			int m_key;
		};

		template<class T>
		struct primitive_converter;

		struct tuple_object_ref;

	} // detail

	class LUABIND_API object
	{

#if !(defined (BOOST_MSVC) && (BOOST_MSVC <= 1200))

	template<class T>
	friend T object_cast(const object& obj);
	template<class T>
	friend struct detail::primitive_converter;

#endif

	friend object get_globals(lua_State*);
	friend object get_registry(lua_State*);
	friend object newtable(lua_State*);
	friend class detail::proxy_object;
	friend class detail::proxy_array_object;
	friend class detail::proxy_raw_object;

	public:

		class array_iterator
		{
		friend class object;
		public:

			typedef std::forward_iterator_tag iterator_category;
			typedef luabind::object value_type;
			typedef value_type& reference;
			typedef value_type* pointer;
			typedef void difference_type;

			array_iterator()
				: m_obj(0)
				, m_key(0)
			{
			}

			array_iterator(const array_iterator& iter)
				: m_obj(iter.m_obj)
				, m_key(iter.m_key)
			{
			}

			~array_iterator() {}

			array_iterator& operator=(const array_iterator& rhs)
			{
				m_obj = rhs.m_obj;
				m_key = rhs.m_key;
				return *this;
			}

			detail::proxy_array_object operator*()
			{
				return m_obj->make_array_proxy(m_key);
			}

			detail::proxy_array_object operator->()
			{
				return m_obj->make_array_proxy(m_key);
			}

			inline array_iterator& operator++()
			{
				LUABIND_CHECK_STACK(m_obj->lua_state());

				m_key++;

				// invalidate the iterator if we hit a nil element
				lua_State* L = m_obj->lua_state();
				m_obj->pushvalue();
				lua_rawgeti(L, -1, m_key);
				if (lua_isnil(L, -1)) m_key = -1;
				lua_pop(L, 2);

				return *this;
			}

			inline array_iterator operator++(int)
			{
				LUABIND_CHECK_STACK(m_obj->lua_state());

				int old_key = m_key;
				m_key++;

				// invalidate the iterator if we hit a nil element
				lua_State* L = m_obj->lua_state();
				m_obj->pushvalue();
				lua_rawgeti(L, -1, m_key);
				if (lua_isnil(L, -1)) m_key = -1;
				lua_pop(L, 2);

				return array_iterator(m_obj, old_key);
			}

			bool operator!=(const array_iterator& rhs) const
			{
				return m_obj != rhs.m_obj || m_key != rhs.m_key;
			}

			bool operator==(const array_iterator& rhs) const
			{
				return !(*this != rhs);
			}

		private:

			array_iterator(object* obj, int key)
				: m_obj(obj)
				, m_key(key)
			{
			}

			object* m_obj;
			int m_key;
		};






		class iterator
		{
		friend class object;
		public:

			typedef std::forward_iterator_tag iterator_category;
			typedef luabind::object value_type;
			typedef value_type& reference;
			typedef value_type* pointer;
			typedef void difference_type;

			iterator()
				: m_obj(0)
			{
			}

			iterator(const iterator& iter)
				: m_obj(iter.m_obj)
			{
				if (m_obj)
				{
					m_key = iter.m_key;
				}
			}

			iterator& operator=(const iterator& rhs)
			{
				m_obj = rhs.m_obj;
				if (m_obj)
				{
					m_key = rhs.m_key;
				}
				else
				{
					m_key.reset();
				}
				return *this;
			}

			detail::proxy_object operator*()
			{
				return m_obj->make_proxy(m_key);
			}

			detail::proxy_object operator->()
			{
				return m_obj->make_proxy(m_key);
			}

			iterator& operator++()
			{
				lua_State* L = m_obj->lua_state();
				LUABIND_CHECK_STACK(L);

				m_obj->pushvalue();
				m_key.get(L);

				if (lua_next(L, -2) != 0)
				{
					lua_pop(L, 1);
					m_key.replace(L);
					lua_pop(L, 1);
				}
				else
				{
					lua_pop(L, 1);
					m_obj = 0;
					m_key.reset();
				}

				return *this;
			}

			bool operator!=(const iterator& rhs) const
			{
				if (m_obj != rhs.m_obj) return true;
				if (m_obj == 0) return false;
				if (m_obj->lua_state() != rhs.m_obj->lua_state()) return true;
				if (m_key.is_valid() != rhs.m_key.is_valid()) return true;

				// TODO: fix this. add a real equality test of the keys
				return true;
			}

			bool operator==(const iterator& rhs) const
			{
				return !(*this != rhs);
			}

			object key() const;

		private:

			iterator(luabind::object* obj, detail::lua_reference const& key)
				: m_obj(obj)
				, m_key(key)
			{
			}

			object* m_obj;
			detail::lua_reference m_key;
		};




		class raw_iterator
		{
		friend class object;
		public:

			typedef std::forward_iterator_tag iterator_category;
			typedef luabind::object value_type;
			typedef value_type& reference;
			typedef value_type* pointer;
			typedef void difference_type;

			raw_iterator()
				: m_obj(0)
			{
			}

			raw_iterator(const raw_iterator& iter)
				: m_obj(iter.m_obj)
				, m_key()
			{
				if (m_obj)
				{
					m_key = iter.m_key;
				}
			}

			raw_iterator& operator=(const raw_iterator& rhs)
			{
				//std::cout << "===\n";
				m_obj = rhs.m_obj;
				if (m_obj)
				{
					m_key = rhs.m_key;
				}
				else
				{
					m_key.reset();
				}
				return *this;
			}

			detail::proxy_raw_object operator*()
			{
				return m_obj->make_raw_proxy(m_key);
			}

			detail::proxy_raw_object operator->()
			{
				return m_obj->make_raw_proxy(m_key);
			}

			raw_iterator& operator++()
			{
				lua_State* L = m_obj->lua_state();
				m_obj->pushvalue();
				m_key.get(L);

				if (lua_next(L, -2) != 0)
				{
					lua_pop(L, 1);
					m_key.replace(L);
					lua_pop(L, 1);
				}
				else
				{
					lua_pop(L, 1);
					m_key.reset();
					m_obj = 0;
				}

				return *this;
			}

			object key() const;

			bool operator!=(const raw_iterator& rhs) const
			{
				if (m_obj != rhs.m_obj) return true;
				if (m_obj == 0) return false;
				if (m_obj->lua_state() != rhs.m_obj->lua_state()) return true;
				if (m_key.is_valid() != rhs.m_key.is_valid()) return true;

				// TODO: fix this. add a real equality test of the keys
				return true;
			}

			bool operator==(const raw_iterator& rhs) const
			{
				return !(*this != rhs);
			}

		private:

			raw_iterator(object* obj, detail::lua_reference const& key)
				: m_obj(obj)
				, m_key(key)
			{}

			object* m_obj;
			detail::lua_reference m_key;
		};






		object()
			: m_state(0)
		{
		}

		explicit object(lua_State* L)
			: m_state(L)
		{
		}

		template<class T>
		object(lua_State* L, const T& val)
			: m_state(L)
		{
			*this = val;
		}

		object(const object& o)
			: m_state(o.m_state)
		{
			o.m_ref.get(m_state);
			m_ref.set(m_state);
		}

		inline ~object()
		{}

		inline bool is_valid() const { return m_ref.is_valid(); }

		// this is a safe substitute for an implicit converter to bool
		typedef void (object::*member_ptr)() const;
		operator member_ptr() const
		{
			if (is_valid()) return &object::dummy;
			return 0;
		}

		int type() const
		{
			pushvalue();
			detail::stack_pop p(lua_state(), 1);
			return lua_type(lua_state(), -1);
		}

		inline iterator begin() const
		{
			LUABIND_CHECK_STACK(m_state);

			m_ref.get(m_state);
			lua_pushnil(m_state);
			detail::stack_pop pop(m_state, 1);
			if (lua_next(m_state, -2) == 0)
				return end();
			lua_pop(m_state, 1);
			detail::lua_reference r;
			r.set(m_state);
			iterator i(const_cast<object*>(this), r);
			return i;
		}

		inline iterator end() const
		{
			return iterator(0, detail::lua_reference());
		}

		inline array_iterator abegin() const
		{
			return array_iterator(const_cast<object*>(this), 1);
		}

		inline array_iterator aend() const
		{
			return array_iterator(const_cast<object*>(this), -1);
		}

		raw_iterator raw_begin() const
		{
			m_ref.get(m_state);
			lua_pushnil(m_state);
			detail::stack_pop pop(m_state, 1);
			if (lua_next(m_state, -2) == 0)
				return raw_end();
			lua_pop(m_state, 1);
			detail::lua_reference r;
			r.set(m_state);
			raw_iterator i(const_cast<object*>(this), r);
			return i;
		}

		raw_iterator raw_end() const
		{
			return raw_iterator(0, detail::lua_reference());
		}

		inline void set() const
		{
			// you are trying to access an invalid object
			assert((m_state != 0) && "you are trying to access an invalid (uninitialized) object");

			allocate_slot();
			m_ref.replace(m_state);
		}
		inline lua_State* lua_state() const { return m_state; }
		inline void pushvalue() const
		{
			// you are trying to dereference an invalid object
			assert((m_ref.is_valid()) && "you are trying to access an invalid (uninitialized) object");
			assert((m_state != 0) && "internal error, please report");
			m_ref.get(m_state);
		}

		void swap(object& rhs);

		template<class T>
		inline object raw_at(const T& key)
		{
			lua_State* L = lua_state();
			pushvalue();
			detail::convert_to_lua(L, key);
			lua_rawget(L, -2);
			detail::lua_reference ref;
			ref.set(L);
			lua_pop(L, 1);
			return object(L, ref, true);
		}

		template<class T>
		inline object at(const T& key)
		{
			LUABIND_CHECK_STACK(m_state);

			lua_State* L = lua_state();
			pushvalue();
			detail::convert_to_lua(L, key);
			lua_gettable(L, -2);
			detail::lua_reference ref;
			ref.set(L);
			lua_pop(L, 1);
			return object(L, ref, true);
		}

		template<class T>
		inline detail::proxy_object operator[](const T& key) const
		{
			LUABIND_CHECK_STACK(m_state);

			detail::convert_to_lua(m_state, key);
			detail::lua_reference ref;
			ref.set(m_state);
			return detail::proxy_object(const_cast<object*>(this), ref);
		}



		// *****************************
		// OPERATOR =

		template<class T>
		object& operator=(const T& val) const
		{
			assert((m_state != 0) && "you cannot assign a non-lua value to an uninitialized object");
			// you cannot assign a non-lua value to an uninitialized object

			detail::convert_to_lua(m_state, val);
			set();
			return const_cast<luabind::object&>(*this);
		}

		object& operator=(const object& o) const;
		object& operator=(const detail::proxy_object& o) const;
		object& operator=(const detail::proxy_raw_object& o) const;
		object& operator=(const detail::proxy_array_object& o) const;

		template<class T, class Policies>
		void assign(const T& val, const Policies& p) const
		{
			assert((m_state != 0) && "you cannot assign a non-lua value to an uninitialized object");
			// you cannot assign a non-lua value to an uninitialized object

			detail::convert_to_lua_p(m_state, val, p);
			set();
		}

		// const overload should return a tuple_object..?
		inline detail::tuple_object_ref operator,(const object& rhs) const;

		// *****************************
		// OPERATOR()

		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/object.hpp>, 1))
		#include BOOST_PP_ITERATE()



		inline detail::proxy_object make_proxy(detail::lua_reference const& key)
		{
			return detail::proxy_object(this, key);
		}

		inline detail::proxy_raw_object make_raw_proxy(detail::lua_reference const& key)
		{
			return detail::proxy_raw_object(this, key);
		}

		inline detail::proxy_array_object make_array_proxy(int key)
		{
			return detail::proxy_array_object(this, key);
		}

		// TODO: it's not possible to make object friend with wrapped_constructor_helper::apply (since
		// it's an inner class), that's why this interface is public
//	private:

		object(lua_State* L, detail::lua_reference const& ref, bool/*, reference*/)
			: m_state(L)
			, m_ref(ref)
		{
		}

private:

		void dummy() const {}

		void allocate_slot() const
		{
			if (!m_ref.is_valid())
			{
				lua_pushboolean(m_state, 0);
				m_ref.set(m_state);
			}
		}

		mutable lua_State* m_state;
		mutable detail::lua_reference m_ref;
	};


	// *************************************
	// OBJECT

	inline void object::swap(object& rhs)
	{
		// you cannot swap objects from different lua states
		assert((lua_state() == rhs.lua_state()) && "you cannot swap objects from different lua states");
		m_ref.swap(rhs.m_ref);
	}

	inline object object::iterator::key() const
	{
		lua_State* L = m_obj->lua_state();
		return object(L, m_key, true);
	}

	inline object object::raw_iterator::key() const
	{
		lua_State* L = m_obj->lua_state();
		return object(L, m_key, true);
	}

	namespace detail
	{
		// tuple object ----------------------------------------------

		struct tuple_object;

		struct tuple_object_ref
		{
			tuple_object_ref(object* a, object* b)
				: n(2)
			{ refs[0] = a; refs[1] = b; }

			tuple_object_ref& operator,(const object& x)
			{ refs[n++] = const_cast<object*>(&x); return *this; }

			struct assign_into
			{
				assign_into() {}

				template<class T>
				assign_into(tuple_object_ref& to, const T& val)
					: target(&to)
					, n(0)
				{ 
					if (n >= target->n) return; 
					*target->refs[n++] = val; 
				}

				template<class T>
				assign_into& operator,(const T& val)
				{ 
					if (n >= target->n) return *this;
					*target->refs[n++] = val; 
					return *this;
				}

				tuple_object_ref* target;
				std::size_t n;
			};

			template<class T>
			assign_into operator=(const T& val)
			{ return assign_into(*this, val); }

			tuple_object_ref(const tuple_object_ref&);
			assign_into operator=(const tuple_object_ref& x)
			{
				for (std::size_t i = 0; i < n && i < x.n; ++i)
					*refs[i] = *x.refs[i];
				return assign_into();
			}

			inline assign_into operator=(const tuple_object&);

			std::size_t n;
			object* refs[10];
		};

		struct tuple_object
		{
			tuple_object(const object& x)
				: n(0)
			{ objs[n++] = x; }

			tuple_object(const tuple_object_ref& x)
			{
				for (std::size_t i = 0; i < x.n; ++i)
					objs[i] = *x.refs[i];
			}

			std::size_t n;
			object objs[10];
		};

		inline tuple_object_ref::assign_into tuple_object_ref::operator=(const tuple_object& x)
		{
			for (std::size_t i = 0; i < n && i < x.n; ++i)
				*refs[i] = x.objs[i];
			return assign_into();
		}

		// *************************************
		// PROXY CALLER

#if !defined(BOOST_MSVC) || (defined(BOOST_MSVC) && (BOOST_MSVC > 1300))
		template<class Tuple>
		template<class Policies>
		luabind::object proxy_caller<Tuple>::operator[](const Policies& p)
		{
			m_called = true;
			lua_State* L = m_obj->lua_state();
			m_obj->pushvalue();
			detail::push_args_from_tuple<1>::apply(L, m_args, p);
			if (pcall(L, boost::tuples::length<Tuple>::value, 1))
			{ 
#ifndef LUABIND_NO_EXCEPTIONS
				throw error(L);
#else
				error_callback_fun e = get_error_callback();
				if (e) e(L);
	
				assert(0 && "the lua function threw an error and exceptions are disabled."
					"if you want to handle this error use luabind::set_error_callback()");
				std::terminate();
#endif
			}
			detail::lua_reference ref;
			ref.set(L);
			return luabind::object(m_obj->lua_state(), ref, true/*luabind::object::reference()*/);
		}
#endif
		// *************************************
		// PROXY OBJECT

#if !defined(BOOST_MSVC) || (defined(BOOST_MSVC) && (BOOST_MSVC > 1300))
		template<class T>
		inline object proxy_object::raw_at(const T& key)
		LUABIND_PROXY_RAW_AT_BODY

		template<class T>
		inline object proxy_object::at(const T& key)
		LUABIND_PROXY_AT_BODY
#endif

		inline lua_State* proxy_object::lua_state() const
		{
			return m_obj->lua_state();
		}

		inline proxy_object::operator luabind::object()
		{
			lua_State* L = m_obj->lua_state();
			pushvalue();
			detail::lua_reference ref;
			ref.set(L);
			return luabind::object(L, ref, true/*luabind::object::reference()*/);
		}


		// *************************************
		// PROXY ARRAY OBJECT

#if !defined(BOOST_MSVC) || (defined(BOOST_MSVC) && (BOOST_MSVC > 1300))
		template<class T>
		inline object proxy_array_object::raw_at(const T& key)
		LUABIND_PROXY_ARRAY_RAW_AT_BODY

		template<class T>
		inline object proxy_array_object::at(const T& key)
		LUABIND_PROXY_ARRAY_AT_BODY
#endif

#undef LUABIND_PROXY_ARRAY_AT_BODY
#undef LUABIND_PROXY_ARRAY_RAW_AT_BODY

		inline lua_State* proxy_array_object::lua_state() const
		{
			return m_obj->lua_state();
		}

		inline proxy_array_object::operator luabind::object()
		{
			lua_State* L = m_obj->lua_state();
			pushvalue();
			detail::lua_reference ref;
			ref.set(L);
			return luabind::object(L, ref, true/*luabind::object::reference()*/);
		}


		// *************************************
		// PROXY RAW OBJECT

#if !defined(BOOST_MSVC) || (defined(BOOST_MSVC) && (BOOST_MSVC > 1300))
		template<class T>
		inline object proxy_raw_object::raw_at(const T& key)
		LUABIND_PROXY_RAW_AT_BODY

		template<class T>
		inline object proxy_raw_object::at(const T& key)
		LUABIND_PROXY_AT_BODY
#endif

#undef LUABIND_PROXY_RAW_AT_BODY
#undef LUABIND_PROXY_AT_BODY

		inline lua_State* proxy_raw_object::lua_state() const
		{
			return m_obj->lua_state();
		}

		inline proxy_raw_object::operator luabind::object()
		{
			lua_State* L = lua_state();
			pushvalue();
			detail::lua_reference ref;
			ref.set(L);
			return luabind::object(L, ref, true/*luabind::object::reference()*/);
		}


		// *************************************
		// PROXY CALLER


		template<class Tuple>
		proxy_caller<Tuple>::~proxy_caller()
		{
			if (m_called) return;

			m_called = true;
			lua_State* L = m_obj->lua_state();
			m_obj->pushvalue();

			push_args_from_tuple<1>::apply(L, m_args);
			if (pcall(L, boost::tuples::length<Tuple>::value, 0))
			{ 
#ifndef LUABIND_NO_EXCEPTIONS
				throw luabind::error(L);
#else
				error_callback_fun e = get_error_callback();
				if (e) e(L);
	
				assert(0 && "the lua function threw an error and exceptions are disabled."
					"if you want to handle this error use luabind::set_error_callback()");
				std::terminate();
#endif
			}
		}

		template<class Tuple>
		proxy_caller<Tuple>::operator luabind::object()
		{
			m_called = true;
			lua_State* L = m_obj->lua_state();
			m_obj->pushvalue();

			push_args_from_tuple<1>::apply(L, m_args);
			if (pcall(L, boost::tuples::length<Tuple>::value, 1))
			{ 
#ifndef LUABIND_NO_EXCEPTIONS
				throw luabind::error(L);
#else
				error_callback_fun e = get_error_callback();
				if (e) e(L);
	
				assert(0 && "the lua function threw an error and exceptions are disabled."
					"if you want to handle this error use luabind::set_error_callback()");
				std::terminate();
#endif
			}
			detail::lua_reference ref;
			ref.set(L);
			return luabind::object(m_obj->lua_state(), ref, true/*luabind::object::reference()*/);
		}

	}

	inline detail::tuple_object_ref object::operator,(const object& rhs) const
	{
		return detail::tuple_object_ref(
			const_cast<object*>(this), const_cast<object*>(&rhs));
	}

	typedef detail::tuple_object function_;

#define LUABIND_DECLARE_OPERATOR(MACRO)\
	MACRO(object, object) \
	MACRO(object, detail::proxy_object) \
	MACRO(object, detail::proxy_array_object) \
	MACRO(object, detail::proxy_raw_object) \
	MACRO(detail::proxy_object, object) \
	MACRO(detail::proxy_object, detail::proxy_object) \
	MACRO(detail::proxy_object, detail::proxy_array_object) \
	MACRO(detail::proxy_object, detail::proxy_raw_object) \
	MACRO(detail::proxy_array_object, object) \
	MACRO(detail::proxy_array_object, detail::proxy_object) \
	MACRO(detail::proxy_array_object, detail::proxy_array_object) \
	MACRO(detail::proxy_array_object, detail::proxy_raw_object) \
	MACRO(detail::proxy_raw_object, object) \
	MACRO(detail::proxy_raw_object, detail::proxy_object) \
	MACRO(detail::proxy_raw_object, detail::proxy_array_object) \
	MACRO(detail::proxy_raw_object, detail::proxy_raw_object)


#define LUABIND_EQUALITY_OPERATOR(lhs, rhs) LUABIND_API bool operator==(const lhs&, const rhs&);
	LUABIND_DECLARE_OPERATOR(LUABIND_EQUALITY_OPERATOR)
#undef LUABIND_EQUALITY_OPERATOR

#define LUABIND_LESSTHAN_OPERATOR(lhs, rhs) LUABIND_API bool operator<(const lhs&, const rhs&);
	LUABIND_DECLARE_OPERATOR(LUABIND_LESSTHAN_OPERATOR)
#undef LUABIND_LESSTHAN_OPERATOR

#define LUABIND_LESSOREQUAL_OPERATOR(lhs_t, rhs_t) LUABIND_API bool operator<=(const lhs_t&, const rhs_t&);
	LUABIND_DECLARE_OPERATOR(LUABIND_LESSOREQUAL_OPERATOR)
#undef LUABIND_LESSOREQUAL_OPERATOR

#define LUABIND_INEQUALITY_OPERATOR(lhs_t, rhs_t)\
	inline bool operator!=(const rhs_t& rhs, const lhs_t& lhs) \
	{ \
		return !(rhs == lhs); \
	}

	LUABIND_DECLARE_OPERATOR(LUABIND_INEQUALITY_OPERATOR)

#undef LUABIND_INEQUALITY_OPERATOR

#define LUABIND_GREATEROREQUAL_OPERATOR(lhs_t, rhs_t)\
	inline bool operator>=(const rhs_t& rhs, const lhs_t& lhs) \
	{ \
		return !(rhs < lhs); \
	}

	LUABIND_DECLARE_OPERATOR(LUABIND_GREATEROREQUAL_OPERATOR)

#undef LUABIND_GREATEROREQUAL_OPERATOR

#define LUABIND_GREATERTHAN_OPERATOR(lhs_t, rhs_t)\
	inline bool operator>(const lhs_t& lhs, const rhs_t& rhs) \
	{ \
		return !(lhs <= rhs); \
	}

	LUABIND_DECLARE_OPERATOR(LUABIND_GREATERTHAN_OPERATOR)
#undef LUABIND_GREATERTHAN_OPERATOR

#undef LUABIND_DECLARE_OPERATOR

}

namespace std
{

#define LUABIND_DEFINE_SWAP(t1,t2)\
	inline void swap(t1 lhs, t2 rhs)\
	{\
			assert((lhs.lua_state() == rhs.lua_state()) && "you cannot swap objects from different lua states");\
			rhs.pushvalue();\
			lhs.pushvalue();\
			rhs.set();\
			lhs.set();\
	}

	inline void swap(luabind::object& lhs, luabind::object& rhs)
	{
		lhs.swap(rhs);
	}

	// object against all other
	LUABIND_DEFINE_SWAP(luabind::object&, const luabind::detail::proxy_object&)
	LUABIND_DEFINE_SWAP(luabind::object&, const luabind::detail::proxy_raw_object&)
	LUABIND_DEFINE_SWAP(luabind::object&, const luabind::detail::proxy_array_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_object&, luabind::object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_raw_object&, luabind::object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_array_object&, luabind::object&)

	// proxy_object against all other
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_object&, const luabind::detail::proxy_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_object&, const luabind::detail::proxy_raw_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_object&, const luabind::detail::proxy_array_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_raw_object&, const luabind::detail::proxy_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_array_object&, const luabind::detail::proxy_object&)

	// proxy_raw_object against all other
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_raw_object&, const luabind::detail::proxy_raw_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_raw_object&, const luabind::detail::proxy_array_object&)
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_array_object&, const luabind::detail::proxy_raw_object&)

	// proxy_array_object against all other
	LUABIND_DEFINE_SWAP(const luabind::detail::proxy_array_object&, const luabind::detail::proxy_array_object&)

#undef LUABIND_DEFINE_SWAP

} // std

#endif // LUABIND_OBJECT_HPP_INCLUDED

#elif BOOST_PP_ITERATION_FLAGS() == 1

#define LUABIND_TUPLE_PARAMS(z, n, data) const A##n *
#define LUABIND_OPERATOR_PARAMS(z, n, data) const A##n & a##n

#if BOOST_PP_ITERATION() > 0
	template<BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
#endif
	detail::proxy_caller<boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> >
	operator()(BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_OPERATOR_PARAMS, _)) const
	{
		typedef boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> tuple_t;
#if BOOST_PP_ITERATION() == 0
		tuple_t args;
#else
		tuple_t args(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), &a));
#endif
		return detail::proxy_caller<tuple_t>(const_cast<luabind::object*>(this), args);
	}

#undef LUABIND_OPERATOR_PARAMS
#undef LUABIND_TUPLE_PARAMS

#endif
