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


#ifndef LUABIND_METHOD_REP_HPP_INCLUDED
#define LUABIND_METHOD_REP_HPP_INCLUDED

#include <luabind/config.hpp>

#include <vector>

#include <luabind/detail/overload_rep.hpp>

namespace luabind { namespace detail
{

	class class_rep;

	/*
		contains information about a method. It contains
		a list of all overloads of the function. If a class
		derives from another class all methods and overloads
		are copied into the derived class and the pointer
		offset is applied to each copied method. The pointer
		offset is needed because if the method is called
		on a derived object the method needs a pointer
		to the base class, and that typecast may need
		an offseted pointer (if multiple inheritance is used).
	*/
	struct method_rep
	{
		void add_overload(const overload_rep& o)
		{
			std::vector<overload_rep>::iterator i = std::find(m_overloads.begin(), m_overloads.end(), o);
			if (i == m_overloads.end())
			{
				// if this overload does not exist, we can just add it to the end of the overloads list
				m_overloads.push_back(o);
			}
			else
			{
				// if this specific overload already exists, replace it
				*i = o;
			}
		}
		const std::vector<overload_rep>& overloads() const throw() { return m_overloads; }

		// this is a pointer to the string kept in class_rep::m_methods, and those strings are deleted
		// at the end of the lua session.
		const char* name;

		// the class_rep in which this method_rep is found
		const class_rep* crep;

	private:
		// this have to be write protected, since each time an overload is
		// added it has to be checked for existence. add_overload() should
		// be used.
		std::vector<overload_rep> m_overloads;
	};

}}

#endif // LUABIND_METHOD_REP_HPP_INCLUDED
