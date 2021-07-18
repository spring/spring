/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ESTRIN_HPP
#define XSIMD_ESTRIN_HPP

#include <tuple>

#include "../types/xsimd_types_include.hpp"

namespace xsimd
{
    namespace detail
    {
        template <class T, uint64_t c>
        inline T coef() noexcept
        {
            using value_type = typename T::value_type;
            return T(caster_t<value_type>(as_unsigned_integer_t<value_type>(c)).f);
        }

        template <class T>
        struct estrin
        {
            T x;

            template <typename... Ts>
            inline T operator()(const Ts&... coefs) noexcept
            {
                return eval(coefs...);
            }

          private:
            inline T eval(const T& c0) noexcept
            {
                return c0;
            }

            inline T eval(const T& c0, const T& c1) noexcept
            {
                return fma(x, c1, c0);
            }

            template <size_t... Is, class Tuple>
            inline T eval(index_sequence<Is...>, const Tuple& tuple)
            {
                return estrin{x * x}(std::get<Is>(tuple)...);
            }

            template <class... Args>
            inline T eval(const std::tuple<Args...>& tuple) noexcept
            {
                return eval(make_index_sequence<sizeof...(Args)>(), tuple);
            }

            template <class... Args>
            inline T eval(const std::tuple<Args...>& tuple, const T& c0) noexcept
            {
                return eval(std::tuple_cat(tuple, std::make_tuple(eval(c0))));
            }

            template <class... Args>
            inline T eval(const std::tuple<Args...>& tuple, const T& c0, const T& c1) noexcept
            {
                return eval(std::tuple_cat(tuple, std::make_tuple(eval(c0, c1))));
            }

            template <class... Args, class... Ts>
            inline T eval(const std::tuple<Args...>& tuple, const T& c0, const T& c1, const Ts&... coefs) noexcept
            {
                return eval(std::tuple_cat(tuple, std::make_tuple(eval(c0, c1))), coefs...);
            }

            template <class... Ts>
            inline T eval(const T& c0, const T& c1, const Ts&... coefs) noexcept
            {
                return eval(std::make_tuple(eval(c0, c1)), coefs...);
            }
        };
    }

    /**********
     * estrin *
     **********/

    template <class T, uint64_t... coefs>
    inline T estrin(const T& x) noexcept
    {
      return detail::estrin<T>{x}(detail::coef<T, coefs>()...);
    }
}

#endif
