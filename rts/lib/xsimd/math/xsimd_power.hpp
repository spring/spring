/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_POWER_HPP
#define XSIMD_POWER_HPP

#include "xsimd_basic_math.hpp"
#include "xsimd_exponential.hpp"
#include "xsimd_fp_manipulation.hpp"
#include "xsimd_fp_sign.hpp"
#include "xsimd_horner.hpp"
#include "xsimd_logarithm.hpp"
#include "xsimd_numerical_constant.hpp"
#include "xsimd/types/xsimd_common_math.hpp"

namespace xsimd
{

    /**
     * Computes the value of the batch \c x raised to the power
     * \c y.
     * @param x batch of floating point values.
     * @param y batch of floating point values.
     * @return \c x raised to the power \c y.
     */
    template <class B>
    batch_type_t<B> pow(const simd_base<B>& x, const simd_base<B>& y);

    // integer specialization
    template <class B, class T1>
    inline typename std::enable_if<std::is_integral<T1>::value, batch_type_t<B>>::type
    pow(const simd_base<B>& t0, const T1& t1);

    /**
     * Computes the cubic root of the batch \c x.
     * @param x batch of floating point values.
     * @return the cubic root of \c x.
     */
    template <class B>
    batch_type_t<B> cbrt(const simd_base<B>& x);

    /**
     * Computes the square root of the sum of the squares of the batches
     * \c x, and \c y.
     * @param x batch of floating point values.
     * @param y batch of floating point values.
     * @return the square root of the sum of the squares of \c x and \c y.
     */
    template <class B>
    batch_type_t<B> hypot(const simd_base<B>& x, const simd_base<B>& y);

    /**********************
     * pow implementation *
     **********************/

    /* origin: boost/simd/arch/common/simd/function/pow.hpp*/
    /*
     * ====================================================
     * copyright 2016 NumScale SAS
     *
     * Distributed under the Boost Software License, Version 1.0.
     * (See copy at http://boost.org/LICENSE_1_0.txt)
     * ====================================================
     */

    namespace detail
    {
        template <class B>
        struct pow_kernel
        {
            static inline B compute(const B& x, const B& y)
            {
                using b_type = B;
                auto negx = x < b_type(0.);
                b_type z = exp(y * log(abs(x)));
                z = select(is_odd(y) && negx, -z, z);
                auto invalid = negx && !(is_flint(y) || xsimd::isinf(y));
                return select(invalid, nan<b_type>(), z);
            }
        };

    }

    template <class B>
    inline batch_type_t<B> pow(const simd_base<B>& x, const simd_base<B>& y)
    {
        return detail::pow_kernel<batch_type_t<B>>::compute(x(), y());
    }

    template <class B, class T1>
    inline typename std::enable_if<std::is_integral<T1>::value, batch_type_t<B>>::type
    pow(const simd_base<B>& t0, const T1& t1)
    {
        return detail::ipow(t0(), t1);
    }

    /***********************
     * cbrt implementation *
     ***********************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/simd/function/cbrt.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B, class T = typename B::value_type>
        struct cbrt_kernel;

        template <class B>
        struct cbrt_kernel<B, float>
        {
            static inline B compute(const B& a)
            {
                B z = abs(a);
#ifndef XSIMD_NO_DENORMALS
                auto denormal = z < smallestposval<B>();
                z = select(denormal, z * twotonmb<B>(), z);
                B f = select(denormal, twotonmbo3<B>(), B(1.));
#endif
                static const B CBRT2 = B(detail::caster32_t(0x3fa14518).f);
                static const B CBRT4 = B(detail::caster32_t(0x3fcb2ff5).f);
                static const B CBRT2I = B(detail::caster32_t(0x3f4b2ff5).f);
                static const B CBRT4I = B(detail::caster32_t(0x3f214518).f);
                using i_type = as_integer_t<B>;
                i_type e;
                B x = frexp(z, e);
                x = horner<B,
                           0x3ece0609,
                           0x3f91eb77,
                           0xbf745265,
                           0x3f0bf0fe,
                           0xbe09e49a>(x);
                auto flag = e >= i_type(0);
                i_type e1 = abs(e);
                i_type rem = e1;
                e1 /= i_type(3);
                rem -= e1 * i_type(3);
                e = e1 * sign(e);
                const B cbrt2 = select(bool_cast(flag), CBRT2, CBRT2I);
                const B cbrt4 = select(bool_cast(flag), CBRT4, CBRT4I);
                B fact = select(bool_cast(rem == i_type(1)), cbrt2, B(1.));
                fact = select(bool_cast(rem == i_type(2)), cbrt4, fact);
                x = ldexp(x * fact, e);
                x -= (x - z / (x * x)) * B(1.f / 3.f);
#ifndef XSIMD_NO_DENORMALS
                x = (x | bitofsign(a)) * f;
#else
                x = x | bitofsign(a);
#endif
#ifndef XSIMD_NO_INFINITIES
                return select(a == B(0.) || xsimd::isinf(a), a, x);
#else
                return select(a == B(0.), a, x);
#endif
            }
        };

        template <class B>
        struct cbrt_kernel<B, double>
        {
            static inline B compute(const B& a)
            {
                B z = abs(a);
#ifndef XSIMD_NO_DENORMALS
                auto denormal = z < smallestposval<B>();
                z = select(denormal, z * twotonmb<B>(), z);
                B f = select(denormal, twotonmbo3<B>(), B(1.));
#endif
                static const B CBRT2 = B(detail::caster64_t(int64_t(0x3ff428a2f98d728b)).f);
                static const B CBRT4 = B(detail::caster64_t(int64_t(0x3ff965fea53d6e3d)).f);
                static const B CBRT2I = B(detail::caster64_t(int64_t(0x3fe965fea53d6e3d)).f);
                static const B CBRT4I = B(detail::caster64_t(int64_t(0x3fe428a2f98d728b)).f);
                using i_type = as_integer_t<B>;
                i_type e;
                B x = frexp(z, e);
                x = horner<B,
                           0x3fd9c0c12122a4feull,
                           0x3ff23d6ee505873aull,
                           0xbfee8a4ca3ba37b8ull,
                           0x3fe17e1fc7e59d58ull,
                           0xbfc13c93386fdff6ull>(x);
                auto flag = e >= zero<i_type>();
                i_type e1 = abs(e);
                i_type rem = e1;
                e1 /= i_type(3);
                rem -= e1 * i_type(3);
                e = e1 * sign(e);
                const B cbrt2 = select(bool_cast(flag), CBRT2, CBRT2I);
                const B cbrt4 = select(bool_cast(flag), CBRT4, CBRT4I);
                B fact = select(bool_cast(rem == i_type(1)), cbrt2, B(1.));
                fact = select(bool_cast(rem == i_type(2)), cbrt4, fact);
                x = ldexp(x * fact, e);
                x -= (x - z / (x * x)) * B(1. / 3.);
                x -= (x - z / (x * x)) * B(1. / 3.);
#ifndef XSIMD_NO_DENORMALS
                x = (x | bitofsign(a)) * f;
#else
                x = x | bitofsign(a);
#endif
#ifndef XSIMD_NO_INFINITIES
                return select(a == B(0.) || xsimd::isinf(a), a, x);
#else
                return select(a == B(0.), a, x);
#endif
            }
        };
    }

    template <class B>
    inline batch_type_t<B> cbrt(const simd_base<B>& x)
    {
        return detail::cbrt_kernel<batch_type_t<B>>::compute(x());
    }

    /************************
     * hypot implementation *
     ************************/

    template <class B>
    inline batch_type_t<B> hypot(const simd_base<B>& x, const simd_base<B>& y)
    {
        return sqrt(fma(x(), x(), y() * y()));
    }
}

#endif
