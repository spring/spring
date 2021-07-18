/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_EXP_REDUCTION_HPP
#define XSIMD_EXP_REDUCTION_HPP

#include "xsimd_horner.hpp"
#include "xsimd_numerical_constant.hpp"
#include "xsimd_rounding.hpp"

namespace xsimd
{
    struct exp_tag
    {
    };
    struct exp2_tag
    {
    };
    struct exp10_tag
    {
    };

    namespace detail
    {

        /**********************
         * exp_reduction_base *
         **********************/

        template <class B, class Tag>
        struct exp_reduction_base;

        template <class B>
        struct exp_reduction_base<B, exp_tag>
        {
            static constexpr B maxlog() noexcept
            {
                return xsimd::maxlog<B>();
            }

            static constexpr B minlog() noexcept
            {
                return xsimd::minlog<B>();
            }
        };

        template <class B>
        struct exp_reduction_base<B, exp2_tag>
        {
            static constexpr B maxlog() noexcept
            {
                return xsimd::maxlog2<B>();
            }

            static constexpr B minlog() noexcept
            {
                return xsimd::minlog2<B>();
            }
        };

        template <class B>
        struct exp_reduction_base<B, exp10_tag>
        {
            static constexpr B maxlog() noexcept
            {
                return xsimd::maxlog10<B>();
            }

            static constexpr B minlog() noexcept
            {
                return xsimd::minlog10<B>();
            }
        };

        /*****************
         * exp_reduction *
         *****************/

        template <class B, class Tag, class T = typename B::value_type>
        struct exp_reduction;

        /* origin: boost/simd/arch/common/detail/generic/f_expo_reduction.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B>
        struct exp_reduction<B, exp_tag, float> : exp_reduction_base<B, exp_tag>
        {
            static inline B approx(const B& x)
            {
                B y = horner<B,
                             0x3f000000,  //  5.0000000e-01
                             0x3e2aa9a5,  //  1.6666277e-01
                             0x3d2aa957,  //  4.1665401e-02
                             0x3c098d8b,  //  8.3955629e-03
                             0x3ab778cf  //  1.3997796e-03
                             >(x);
                return ++fma(y, x * x, x);
            }

            static inline B reduce(const B& a, B& x)
            {
                B k = nearbyint(invlog_2<B>() * a);
                x = fnma(k, log_2hi<B>(), a);
                x = fnma(k, log_2lo<B>(), x);
                return k;
            }
        };

        template <class B>
        struct exp_reduction<B, exp2_tag, float> : exp_reduction_base<B, exp2_tag>
        {
            static inline B approx(const B& x)
            {
                B y = horner<B,
                             0x3e75fdf1,  //    2.4022652e-01
                             0x3d6356eb,  //    5.5502813e-02
                             0x3c1d9422,  //    9.6178371e-03
                             0x3ab01218,  //    1.3433127e-03
                             0x3922c8c4  //    1.5524315e-04
                             >(x);
                return ++fma(y, x * x, x * log_2<B>());
            }

            static inline B reduce(const B& a, B& x)
            {
                B k = nearbyint(a);
                x = (a - k);
                return k;
            }
        };

        template <class B>
        struct exp_reduction<B, exp10_tag, float> : exp_reduction_base<B, exp10_tag>
        {
            static inline B approx(const B& x)
            {
                return ++(horner<B,
                                 0x40135d8e,  //    2.3025851e+00
                                 0x4029a926,  //    2.6509490e+00
                                 0x400237da,  //    2.0346589e+00
                                 0x3f95eb4c,  //    1.1712432e+00
                                 0x3f0aacef,  //    5.4170126e-01
                                 0x3e54dff1  //    2.0788552e-01
                                 >(x) *
                          x);
            }

            static inline B reduce(const B& a, B& x)
            {
                B k = nearbyint(invlog10_2<B>() * a);
                x = fnma(k, log10_2hi<B>(), a);
                x -= k * log10_2lo<B>();
                return k;
            }
        };

        /* origin: boost/simd/arch/common/detail/generic/d_expo_reduction.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B>
        struct exp_reduction<B, exp_tag, double> : exp_reduction_base<B, exp_tag>
        {
            static inline B approx(const B& x)
            {
                B t = x * x;
                return fnma(t,
                            horner<B,
                                   0x3fc555555555553eull,
                                   0xbf66c16c16bebd93ull,
                                   0x3f11566aaf25de2cull,
                                   0xbebbbd41c5d26bf1ull,
                                   0x3e66376972bea4d0ull>(t),
                            x);
            }

            static inline B reduce(const B& a, B& hi, B& lo, B& x)
            {
                B k = nearbyint(invlog_2<B>() * a);
                hi = fnma(k, log_2hi<B>(), a);
                lo = k * log_2lo<B>();
                x = hi - lo;
                return k;
            }

            static inline B finalize(const B& x, const B& c, const B& hi, const B& lo)
            {
                return B(1.) - (((lo - (x * c) / (B(2.) - c)) - hi));
            }
        };

        template <class B>
        struct exp_reduction<B, exp2_tag, double> : exp_reduction_base<B, exp2_tag>
        {
            static inline B approx(const B& x)
            {
                B t = x * x;
                return fnma(t,
                            horner<B,
                                   0x3fc555555555553eull,
                                   0xbf66c16c16bebd93ull,
                                   0x3f11566aaf25de2cull,
                                   0xbebbbd41c5d26bf1ull,
                                   0x3e66376972bea4d0ull>(t),
                            x);
            }

            static inline B reduce(const B& a, B&, B&, B& x)
            {
                B k = nearbyint(a);
                x = (a - k) * log_2<B>();
                return k;
            }

            static inline B finalize(const B& x, const B& c, const B&, const B&)
            {
                return B(1.) + x + x * c / (B(2.) - c);
            }
        };

        template <class B>
        struct exp_reduction<B, exp10_tag, double> : exp_reduction_base<B, exp10_tag>
        {
            static inline B approx(const B& x)
            {
                B xx = x * x;
                B px = x * horner<B,
                                  0x40a2b4798e134a01ull,
                                  0x40796b7a050349e4ull,
                                  0x40277d9474c55934ull,
                                  0x3fa4fd75f3062dd4ull>(xx);
                B x2 = px / (horner1<B,
                                     0x40a03f37650df6e2ull,
                                     0x4093e05eefd67782ull,
                                     0x405545fdce51ca08ull>(xx) -
                             px);
                return ++(x2 + x2);
            }

            static inline B reduce(const B& a, B&, B&, B& x)
            {
                B k = nearbyint(invlog10_2<B>() * a);
                x = fnma(k, log10_2hi<B>(), a);
                x = fnma(k, log10_2lo<B>(), x);
                return k;
            }

            static inline B finalize(const B&, const B& c, const B&, const B&)
            {
                return c;
            }
        };
    }
}

#endif
