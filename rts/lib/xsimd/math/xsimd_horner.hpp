/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_HORNER_HPP
#define XSIMD_HORNER_HPP

#include "../types/xsimd_types_include.hpp"
#include "xsimd_estrin.hpp"

namespace xsimd
{

    /**********
     * horner *
     **********/

    /* origin: boost/simdfunction/horn.hpp*/
    /*
     * ====================================================
     * copyright 2016 NumScale SAS
     *
     * Distributed under the Boost Software License, Version 1.0.
     * (See copy at http://boost.org/LICENSE_1_0.txt)
     * ====================================================
     */

    template <class T>
    inline T horner(const T&) noexcept
    {
        return T(0.);
    }

    template <class T, uint64_t c0>
    inline T horner(const T&) noexcept
    {
        return detail::coef<T, c0>();
    }

    template <class T, uint64_t c0, uint64_t c1, uint64_t... args>
    inline T horner(const T& x) noexcept
    {
        return fma(x, horner<T, c1, args...>(x), detail::coef<T, c0>());
    }

    /***********
     * horner1 *
     ***********/

    /* origin: boost/simdfunction/horn1.hpp*/
    /*
     * ====================================================
     * copyright 2016 NumScale SAS
     *
     * Distributed under the Boost Software License, Version 1.0.
     * (See copy at http://boost.org/LICENSE_1_0.txt)
     * ====================================================
     */

    template <class T>
    inline T horner1(const T&) noexcept
    {
        return T(1.);
    }

    template <class T, uint64_t c0>
    inline T horner1(const T& x) noexcept
    {
        return x + detail::coef<T, c0>();
    }

    template <class T, uint64_t c0, uint64_t c1, uint64_t... args>
    inline T horner1(const T& x) noexcept
    {
        return fma(x, horner1<T, c1, args...>(x), detail::coef<T, c0>());
    }
}

#endif
