/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_HYPERBOLIC_HPP
#define XSIMD_HYPERBOLIC_HPP

#include <type_traits>

#include "xsimd_exponential.hpp"
#include "xsimd_fp_sign.hpp"
#include "xsimd_logarithm.hpp"
#include "xsimd_power.hpp"

namespace xsimd
{

    template <class B>
    batch_type_t<B> average(const simd_base<B>& x1, const simd_base<B>& x2);

    /**
     * Computes the hyperbolic sine of the batch \c x.
     * @param x batch of floating point values.
     * @return the hyperbolic sine of \c x.
     */
    template <class B>
    batch_type_t<B> sinh(const simd_base<B>& x);

    /**
     * Computes the hyperbolic cosine of the batch \c x.
     * @param x batch of floating point values.
     * @return the hyperbolic cosine of \c x.
     */
    template <class B>
    batch_type_t<B> cosh(const simd_base<B>& x);

    /**
     * Computes the hyperbolic tangent of the batch \c x.
     * @param x batch of floating point values.
     * @return the hyperbolic tangent of \c x.
     */
    template <class B>
    batch_type_t<B> tanh(const simd_base<B>& x);

    /**
     * Computes the inverse hyperbolic sine of the batch \c x.
     * @param x batch of floating point values.
     * @return the inverse hyperbolic sine of \c x.
     */
    template <class B>
    batch_type_t<B> asinh(const simd_base<B>& x);

    /**
     * Computes the inverse hyperbolic cosine of the batch \c x.
     * @param x batch of floating point values.
     * @return the inverse hyperbolic cosine of \c x.
     */
    template <class B>
    batch_type_t<B> acosh(const simd_base<B>& x);

    /**
     * Computes the inverse hyperbolic tangent of the batch \c x.
     * @param x batch of floating point values.
     * @return the inverse hyperbolic tangent of \c x.
     */
    template <class B>
    batch_type_t<B> atanh(const simd_base<B>& x);

    /***************************
     * average  implementation *
     ***************************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/simd/function/average.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B, bool cond = std::is_floating_point<typename B::value_type>::value>
        struct average_impl
        {
            static inline B compute(const B& x1, const B& x2)
            {
                return (x1 & x2) + ((x1 ^ x2) >> 1);
            }
        };

        template <class B>
        struct average_impl<B, true>
        {
            static inline B compute(const B& x1, const B& x2)
            {
                return fma(x1, B(0.5), x2 * B(0.5));
            }
        };
    }

    template <class B>
    inline batch_type_t<B> average(const simd_base<B>& x1, const simd_base<B>& x2)
    {
        return detail::average_impl<batch_type_t<B>>::compute(x1(), x2());
    }

    /***********************
     * sinh implementation *
     ***********************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/detail/generic/sinh_kernel.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B, class T = typename B::value_type>
        struct sinh_kernel_impl;

        template <class B>
        struct sinh_kernel_impl<B, float>
        {
            static inline B compute(const B& x)
            {
                B sqrx = x * x;
                return horner<B,
                              0x3f800000,  // 1.0f
                              0x3e2aaacc,  // 1.66667160211E-1f
                              0x3c087bbe,  // 8.33028376239E-3f
                              0x39559e2f  // 2.03721912945E-4f
                              >(sqrx) *
                    x;
            }
        };

        template <class B>
        struct sinh_kernel_impl<B, double>
        {
            static inline B compute(const B& x)
            {
                B sqrx = x * x;
                return fma(x, (horner<B,
                                      0xc115782bdbf6ab05ull,  //  -3.51754964808151394800E5
                                      0xc0c694b8c71d6182ull,  //  -1.15614435765005216044E4,
                                      0xc064773a398ff4feull,  //  -1.63725857525983828727E2,
                                      0xbfe9435fe8bb3cd6ull  //  -7.89474443963537015605E-1
                                      >(sqrx) /
                               horner1<B,
                                       0xc1401a20e4f90044ull,  //  -2.11052978884890840399E6
                                       0x40e1a7ba7ed72245ull,  //   3.61578279834431989373E4,
                                       0xc0715b6096e96484ull  //  -2.77711081420602794433E2,
                                       >(sqrx)) *
                               sqrx,
                           x);
            }
        };

        template <class B>
        struct sinh_kernel
        {
            static inline B compute(const B& a)
            {
                using b_type = B;
                b_type half = b_type(0.5);
                b_type x = abs(a);
                auto lt1 = x < b_type(1.);
                b_type bts = bitofsign(a);
                b_type z(0.);
                if (any(lt1))
                {
                    z = sinh_kernel_impl<b_type>::compute(x);
                    if (all(lt1))
                        return z ^ bts;
                }
                auto test1 = x >(maxlog<b_type>() - log_2<b_type>());
                b_type fac = select(test1, half, b_type(1.));
                b_type tmp = exp(x * fac);
                b_type tmp1 = half * tmp;
                b_type r = select(test1, tmp1 * tmp, tmp1 - half / tmp);
                return select(lt1, z, r) ^ bts;
            }
        };
    }

    /* origin: boost/simd/arch/common/simd/function/sinh.hpp */
    /*
     * ====================================================
     * copyright 2016 NumScale SAS
     *
     * Distributed under the Boost Software License, Version 1.0.
     * (See copy at http://boost.org/LICENSE_1_0.txt)
     * ====================================================
     */
    template <class B>
    inline batch_type_t<B> sinh(const simd_base<B>& a)
    {
        return detail::sinh_kernel<batch_type_t<B>>::compute(a());
    }

    /***********************
     * cosh implementation *
     ***********************/

    /* origin: boost/simd/arch/common/simd/function/cosh.hpp */
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
        struct cosh_kernel
        {
            static inline B compute(const B& a)
            {
                using b_type = B;
                b_type x = abs(a);
                auto test1 = x > (maxlog<b_type>() - log_2<b_type>());
                b_type fac = select(test1, b_type(0.5), b_type(1.));
                b_type tmp = exp(x * fac);
                b_type tmp1 = b_type(0.5) * tmp;
                return select(test1, tmp1 * tmp, average(tmp, b_type(1.) / tmp));
            }
        };
    }

    template <class B>
    inline batch_type_t<B> cosh(const simd_base<B>& a)
    {
        return detail::cosh_kernel<batch_type_t<B>>::compute(a());
    }

    /***********************
     * tanh implementation *
     ***********************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/detail/generic/tanh_kernel.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */
        template <class B, class T = typename B::value_type>
        struct tanh_kernel_impl;

        template <class B>
        struct tanh_kernel_impl<B, float>
        {
            static inline B tanh(const B& x)
            {
                B sqrx = x * x;
                return fma(horner<B,
                                  0xbeaaaa99,  //    -3.33332819422E-1F
                                  0x3e088393,  //    +1.33314422036E-1F
                                  0xbd5c1e2d,  //    -5.37397155531E-2F
                                  0x3ca9134e,  //    +2.06390887954E-2F
                                  0xbbbaf0ea  //    -5.70498872745E-3F
                                  >(sqrx) *
                               sqrx,
                           x, x);
            }

            static inline B cotanh(const B& x)
            {
                return B(1.) / tanh(x);
            }
        };

        template <class B>
        struct tanh_kernel_impl<B, double>
        {
            static inline B tanh(const B& x)
            {
                B sqrx = x * x;
                return fma(sqrx * p(sqrx) / q(sqrx), x, x);
            }

            static inline B cotanh(const B& x)
            {
                B sqrx = x * x;
                B qval = q(sqrx);
                return qval / (x * fma(p(sqrx), sqrx, qval));
            }

            static inline B p(const B& x)
            {
                return horner<B,
                              0xc0993ac030580563,  // -1.61468768441708447952E3
                              0xc058d26a0e26682d,  // -9.92877231001918586564E1,
                              0xbfeedc5baafd6f4b  // -9.64399179425052238628E-1
                              >(x);
            }

            static inline B q(const B& x)
            {
                return horner1<B,
                               0x40b2ec102442040c,  //  4.84406305325125486048E3
                               0x40a176fa0e5535fa,  //  2.23548839060100448583E3,
                               0x405c33f28a581B86  //  1.12811678491632931402E2,
                               >(x);
            }
        };

        template <class B>
        struct tanh_kernel
        {
            static inline B compute(const B& a)
            {
                using b_type = B;
                b_type one(1.);
                b_type x = abs(a);
                auto test = x < (b_type(5.) / b_type(8.));
                b_type bts = bitofsign(a);
                b_type z = one;
                if (any(test))
                {
                    z = tanh_kernel_impl<b_type>::tanh(x);
                    if (all(test))
                        return z ^ bts;
                }
                b_type r = fma(b_type(-2.), one / (one + exp(x + x)), one);
                return select(test, z, r) ^ bts;
            }
        };
    }

    /* origin: boost/simd/arch/common/simd/function/tanh.hpp */
    /*
     * ====================================================
     * copyright 2016 NumScale SAS
     *
     * Distributed under the Boost Software License, Version 1.0.
     * (See copy at http://boost.org/LICENSE_1_0.txt)
     * ====================================================
     */

    template <class B>
    inline batch_type_t<B> tanh(const simd_base<B>& a)
    {
        return detail::tanh_kernel<batch_type_t<B>>::compute(a());
    }

    /***********************
     * sinh implementation *
     ***********************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/simd/function/asinh.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B, class T = typename B::value_type>
        struct asinh_kernel;

        template <class B>
        struct asinh_kernel<B, float>
        {
            static inline B compute(const B& a)
            {
                B x = abs(a);
                auto lthalf = x < B(0.5);
                B x2 = x * x;
                B bts = bitofsign(a);
                B z(0.);
                if (any(lthalf))
                {
                    z = horner<B,
                               0x3f800000,
                               0xbe2aa9ad,
                               0x3d9949b1,
                               0xbd2ee581,
                               0x3ca4d6e6>(x2) *
                        x;
                    if (all(lthalf))
                        return z ^ bts;
                }
                B tmp = select(x > oneosqrteps<B>(), x, average(x, hypot(B(1.), x)));
#ifndef XSIMD_NO_NANS
                return select(xsimd::isnan(a), nan<B>(), select(lthalf, z, log(tmp) + log_2<B>()) ^ bts);
#else
                return select(lthalf, z, log(tmp) + log_2<B>()) ^ bts;
#endif
            }
        };

        template <class B>
        struct asinh_kernel<B, double>
        {
            static inline B compute(const B& a)
            {
                B x = abs(a);
                auto test = x > oneosqrteps<B>();
                B z = select(test, x - B(1.), x + x * x / (B(1.) + hypot(B(1.), x)));
#ifndef XSIMD_NO_INFINITIES
                z = select(x == infinity<B>(), x, z);
#endif
                B l1pz = log1p(z);
                z = select(test, l1pz + log_2<B>(), l1pz);
                return bitofsign(a) ^ z;
            }
        };
    }

    template <class B>
    inline batch_type_t<B> asinh(const simd_base<B>& x)
    {
        return detail::asinh_kernel<batch_type_t<B>>::compute(x());
    }

    /************************
     * acosh implementation *
     ************************/

    /* origin: boost/simd/arch/common/simd/function/acosh.hpp */
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
        struct acosh_kernel
        {
            static inline B compute(const B& a)
            {
                using b_type = B;
                b_type x = a - b_type(1.);
                auto test = x > oneotwoeps<b_type>();
                b_type z = select(test, a, x + sqrt(x + x + x * x));
                b_type l1pz = log1p(z);
                return select(test, l1pz + log_2<b_type>(), l1pz);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> acosh(const simd_base<B>& a)
    {
        return detail::acosh_kernel<batch_type_t<B>>::compute(a());
    }

    /************************
     * atanh implementation *
     ************************/

    /* origin: boost/simd/arch/common/simd/function/acosh.hpp */
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
        struct atanh_kernel
        {
            static inline B compute(const B& a)
            {
                using b_type = B;
                b_type x = abs(a);
                b_type t = x + x;
                b_type z = b_type(1.) - x;
                auto test = x < b_type(0.5);
                b_type tmp = select(test, x, t) / z;
                return bitofsign(a) ^ (b_type(0.5) * log1p(select(test, fma(t, tmp, t), tmp)));
            }
        };
    }

    template <class B>
    inline batch_type_t<B> atanh(const simd_base<B>& a)
    {
        return detail::atanh_kernel<batch_type_t<B>>::compute(a());
    }
}

#endif
