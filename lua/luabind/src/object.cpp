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

#include <luabind/lua_include.hpp>

#include <luabind/luabind.hpp>
#include <luabind/detail/implicit_cast.hpp>

using namespace luabind::detail;

namespace luabind
{
	namespace detail
	{


		// *************************************
		// PROXY OBJECT

#define LUABIND_PROXY_ASSIGNMENT_OPERATOR(rhs)\
		proxy_object& proxy_object::operator=(const rhs& p) \
		{ \
			assert((lua_state() == p.lua_state()) && "you cannot assign a value from a different lua state"); \
			lua_State* L = lua_state(); \
			m_obj->pushvalue(); \
			m_key.get(L);\
			p.pushvalue(); \
			lua_settable(L, -3); \
			lua_pop(L, 1); \
			return *this; \
		}

LUABIND_PROXY_ASSIGNMENT_OPERATOR(object)
LUABIND_PROXY_ASSIGNMENT_OPERATOR(proxy_object)
LUABIND_PROXY_ASSIGNMENT_OPERATOR(proxy_raw_object)
LUABIND_PROXY_ASSIGNMENT_OPERATOR(proxy_array_object)

#undef LUABIND_PROXY_ASSIGNMENT_OPERATOR

		void proxy_object::pushvalue() const
		{
			assert((m_key.is_valid()) && "you cannot call pushvalue() on an uninitialized object");

			lua_State* L = m_obj->lua_state();
			m_obj->pushvalue();
			m_key.get(L);
			lua_gettable(L, -2);
			// remove the table and leave the value on top of the stack
			lua_remove(L, -2);
		}

		void proxy_object::set() const
		{
			lua_State* L = lua_state();
			m_obj->pushvalue();
			m_key.get(L);
			lua_pushvalue(L, -3);
			lua_settable(L, -3);
			// pop table and value
			lua_pop(L, 2);
		}



		// *************************************
		// PROXY ARRAY OBJECT



#define LUABIND_ARRAY_PROXY_ASSIGNMENT_OPERATOR(rhs)\
		proxy_array_object& proxy_array_object::operator=(const rhs& p) \
		{ \
			assert((lua_state() == p.lua_state()) && "you cannot assign a value from a different lua state"); \
			lua_State* L = lua_state(); \
			m_obj->pushvalue(); \
			p.pushvalue(); \
			lua_rawseti(L, -2, m_key); \
			lua_pop(L, 1); \
			return *this; \
		}

LUABIND_ARRAY_PROXY_ASSIGNMENT_OPERATOR(object)
LUABIND_ARRAY_PROXY_ASSIGNMENT_OPERATOR(proxy_object)
LUABIND_ARRAY_PROXY_ASSIGNMENT_OPERATOR(proxy_raw_object)
LUABIND_ARRAY_PROXY_ASSIGNMENT_OPERATOR(proxy_array_object)

#undef LUABIND_ARRAY_PROXY_ASSIGNMENT_OPERATOR

		void proxy_array_object::pushvalue() const
		{
			// you are trying to dereference an invalid object
			assert((m_key != -1) && "you cannot call pushvalue() on an uninitialized object");

			lua_State* L = m_obj->lua_state();
			m_obj->pushvalue();
			lua_rawgeti(L, -1, m_key);
			lua_remove(L, -2);
		}

		void proxy_array_object::set() const
		{
			lua_State* L = lua_state();
			m_obj->pushvalue();
			lua_pushvalue(L, -2);
			lua_rawseti(L, -2, m_key);
			// pop table and value
			lua_pop(L, 2);
		}


		// *************************************
		// PROXY RAW OBJECT


#define LUABIND_RAW_PROXY_ASSIGNMENT_OPERATOR(rhs)\
		proxy_raw_object& proxy_raw_object::operator=(const rhs& p) \
		{ \
			assert((lua_state() == p.lua_state()) && "you cannot assign a value from a different lua state"); \
			lua_State* L = lua_state(); \
			m_obj->pushvalue(); \
			m_key.get(L);\
			p.pushvalue(); \
			lua_rawset(L, -3); \
			lua_pop(L, 1); \
			return *this; \
		}

LUABIND_RAW_PROXY_ASSIGNMENT_OPERATOR(object)
LUABIND_RAW_PROXY_ASSIGNMENT_OPERATOR(proxy_object)
LUABIND_RAW_PROXY_ASSIGNMENT_OPERATOR(proxy_raw_object)
LUABIND_RAW_PROXY_ASSIGNMENT_OPERATOR(proxy_array_object)

#undef LUABIND_RAW_PROXY_ASSIGNMENT_OPERATOR

		void proxy_raw_object::pushvalue() const
		{
			assert((m_key.is_valid()) && "you cannot call pushvalue() on an uninitiallized object");

			lua_State* L = lua_state();
			m_obj->pushvalue();
			m_key.get(L);
			lua_rawget(L, -2);
			// remove the table and leave the value on top of the stack
			lua_remove(L, -2);
		}

		void proxy_raw_object::set() const
		{
			lua_State* L = lua_state();
			m_obj->pushvalue();
			m_key.get(L);
			lua_pushvalue(L, -3);
			lua_rawset(L, -3);
			// pop table and value
			lua_pop(L, 2);
		}

	} // detail

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


		// *****************************
		// OPERATOR ==



#define LUABIND_EQUALITY_OPERATOR(lhs_t, rhs_t)\
	bool operator==(const lhs_t& lhs, const rhs_t& rhs) \
	{ \
		if (!lhs.is_valid()) return false; \
		if (!rhs.is_valid()) return false; \
		assert((lhs.lua_state() == rhs.lua_state()) && "you cannot compare objects from different lua states"); \
		lua_State* L = lhs.lua_state(); \
		lhs.pushvalue(); \
		rhs.pushvalue(); \
		bool result = lua_equal(L, -1, -2) != 0; \
		lua_pop(L, 2); \
		return result; \
	}

	LUABIND_DECLARE_OPERATOR(LUABIND_EQUALITY_OPERATOR)

#undef LUABIND_EQUALITY_OPERATOR


	// *****************************
	// OPERATOR =

#define LUABIND_ASSIGNMENT_OPERATOR(rhs)\
	object& object::operator=(const rhs& o) const \
	{ \
		m_state = o.lua_state(); \
		o.pushvalue(); \
		set(); \
		return const_cast<luabind::object&>(*this); \
	}

	LUABIND_ASSIGNMENT_OPERATOR(object)
	LUABIND_ASSIGNMENT_OPERATOR(proxy_object)
	LUABIND_ASSIGNMENT_OPERATOR(proxy_array_object)
	LUABIND_ASSIGNMENT_OPERATOR(proxy_raw_object)


	
		// *****************************
		// OPERATOR ==



#define LUABIND_LESSTHAN_OPERATOR(lhs_t, rhs_t)\
	bool operator<(const lhs_t& lhs, const rhs_t& rhs) \
	{ \
		if (!lhs.is_valid()) return false; \
		if (!rhs.is_valid()) return false; \
		assert((lhs.lua_state() == rhs.lua_state()) && "you cannot compare objects from different lua states"); \
		lua_State* L = lhs.lua_state(); \
		lhs.pushvalue(); \
		rhs.pushvalue(); \
		bool result = lua_lessthan(L, -1, -2) != 0; \
		lua_pop(L, 2); \
		return result; \
	}

	LUABIND_DECLARE_OPERATOR(LUABIND_LESSTHAN_OPERATOR)

#undef LUABIND_LESSTHAN_OPERATOR

#define LUABIND_LESSOREQUAL_OPERATOR(lhs_t, rhs_t)\
	bool operator<=(const lhs_t& lhs, const rhs_t& rhs) \
	{ \
		if (!lhs.is_valid()) return false; \
		if (!rhs.is_valid()) return false; \
		assert((lhs.lua_state() == rhs.lua_state()) && "you cannot compare objects from different lua states"); \
		lua_State* L = lhs.lua_state(); \
		lhs.pushvalue(); \
		rhs.pushvalue(); \
		bool result1 = lua_lessthan(L, -1, -2) != 0; \
		bool result2 = lua_equal(L, -1, -2) != 0; \
		lua_pop(L, 2); \
		return result1 || result2; \
	}

	LUABIND_DECLARE_OPERATOR(LUABIND_LESSOREQUAL_OPERATOR)

#undef LUABIND_LESSOREQUAL_OPERATOR

#undef LUABIND_GREATERTHAN_OPERATOR


#undef LUABIND_DECLARE_OPERATOR

}

