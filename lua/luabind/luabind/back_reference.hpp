// Copyright (c) 2004 Daniel Wallin and Arvid Norberg

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

#ifndef BACK_REFERENCE_040510_HPP
#define BACK_REFERENCE_040510_HPP

#include <luabind/lua_include.hpp>
#include <luabind/wrapper_base.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/mpl/if.hpp>

namespace luabind {

#if !(defined(BOOST_MSVC) && BOOST_MSVC <= 1300)
    template<class T>
    T* get_pointer(T& ref)
    {
        return &ref;
    }

    namespace detail {
#else
    namespace detail {

        struct no_overload_tag {};

    } // namespace detail

    detail::no_overload_tag get_pointer(...);

    namespace detail {

    typedef char(&yes_overload)[1];
    typedef char(&no_overload)[2];

    no_overload check_overload(no_overload_tag);
    template<class T>
    yes_overload check_overload(T const&);

#endif

    template<class T>
    struct extract_wrap_base
    {
# if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
        BOOST_STATIC_CONSTANT(bool,
            value = sizeof(check_overload(get_pointer(*(T*)0)))
                 == sizeof(yes_overload)
        );

        static wrap_base const* extract(T const* ptr)
        {
            return extract_impl(ptr, boost::mpl::bool_<value>());
        }

        static wrap_base const* extract_impl(T const* ptr, boost::mpl::true_)
        {
            return dynamic_cast<wrap_base const*>(
                get_pointer(*ptr));
        }

        static wrap_base const* extract_impl(T const* ptr, boost::mpl::false_)
        {
            return dynamic_cast<wrap_base const*>(ptr);
        }

        static wrap_base* extract(T* ptr)
        {
            return extract_impl(ptr, boost::mpl::bool_<value>());
        }

        static wrap_base* extract_impl(T* ptr, boost::mpl::true_)
        {
            return dynamic_cast<wrap_base*>(
                get_pointer(*ptr));
        }

        static wrap_base* extract_impl(T* ptr, boost::mpl::false_)
        {
            return dynamic_cast<wrap_base*>(ptr);
        }
# else       
       static wrap_base const* extract(T const* ptr)
        {
            return dynamic_cast<wrap_base const*>(get_pointer(*ptr));
        }

        static wrap_base* extract(T* ptr)
        {
            return dynamic_cast<wrap_base*>(get_pointer(*ptr));
        }
# endif
    };

    struct default_back_reference {};

    template<class T>
    struct back_reference_impl : default_back_reference
    {
        static bool extract(lua_State* L, T const* ptr)
        {
            if (!has_wrapper) return false;

            if (wrap_base const* p = extract_wrap_base<T>::extract(ptr))
            {
                wrap_access::ref(*p).get(L);
                return true;
            }

            return false;
        }

        static bool move(lua_State* L, T* ptr)
        {
            if (!has_wrapper) return false;

            if (wrap_base* p = extract_wrap_base<T>::extract(ptr))
            {
                assert(wrap_access::ref(*p).m_strong_ref.is_valid());
                wrap_access::ref(*p).get(L);
                wrap_access::ref(*p).m_strong_ref.reset();
                return true;
            }

            return false;
        }

        static bool has_wrapper;
    };

    template<class T>
    bool back_reference_impl<T>::has_wrapper = false;

    template<class T>
    struct back_reference_do_nothing
    {
        static bool extract(lua_State*, T const*)
        {
            return false;
        }

        static bool move(lua_State*, T*)
        {
            return false;
        }
    };

    } // namespace detail

#ifndef LUABIND_NO_RTTI

    template<class T>
    struct back_reference
        : boost::mpl::if_<
              boost::is_polymorphic<T>
            , detail::back_reference_impl<T>
            , detail::back_reference_do_nothing<T>
          >::type
    {
    };

#else

    template<class T>
    struct back_reference
        : detail::back_reference_do_nothing<T>
    {
    };

#endif

} // namespace luabind

#endif // BACK_REFERENCE_040510_HPP

