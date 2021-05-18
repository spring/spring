/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_LOGARITHM_HPP
#define XSIMD_LOGARITHM_HPP

#include "xsimd_numerical_constant.hpp"

namespace xsimd
{
    /**
     * Computes the natural logarithm of the batch \c x.
     * @param x batch of floating point values.
     * @return the natural logarithm of \c x.
     */
    template <class B>
    batch_type_t<B> log(const simd_base<B>& x);

    /**
     * Computes the base 2 logarithm of the batch \c x.
     * @param x batch of floating point values.
     * @return the base 2 logarithm of \c x.
     */
    template <class B>
    batch_type_t<B> log2(const simd_base<B>& x);

    /**
     * Computes the base 10 logarithm of the batch \c x.
     * @param x batch of floating point values.
     * @return the base 10 logarithm of \c x.
     */
    template <class B>
    batch_type_t<B> log10(const simd_base<B>& x);

    /**
     * Computes the natural logarithm of one plus the batch \c x.
     * @param x batch of floating point values.
     * @return the natural logarithm of one plus \c x.
     */
    template <class B>
    batch_type_t<B> log1p(const simd_base<B>& x);

    /**********************
     * log implementation *
     **********************/

    namespace detail
    {
        template <class B, class T = typename B::value_type>
        struct log_kernel;

        template <class B>
        struct log_kernel<B, float>
        {
            /* origin: boost/simd/arch/common/simd/function/log.hpp */
            /*
             * ====================================================
             * copyright 2016 NumScale SAS
             *
             * Distributed under the Boost Software License, Version 1.0.
             * (See copy at http://boost.org/LICENSE_1_0.txt)
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                using i_type = as_integer_t<B>;
                B x = a;
                i_type k(0);
                auto isnez = (a != B(0.));
#ifndef XSIMD_NO_DENORMALS
                auto test = (a < smallestposval<B>()) && isnez;
                if (any(test))
                {
                    k = select(bool_cast(test), k - i_type(23), k);
                    x = select(test, x * B(8388608ul), x);
                }
#endif
                i_type ix = bitwise_cast<i_type>(x);
                ix += 0x3f800000 - 0x3f3504f3;
                k += (ix >> 23) - 0x7f;
                ix = (ix & i_type(0x007fffff)) + 0x3f3504f3;
                x = bitwise_cast<B>(ix);
                B f = --x;
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B, 0x3eccce13, 0x3e789e26>(w);
                B t2 = z * horner<B, 0x3f2aaaaa, 0x3e91e9ee>(w);
                B R = t2 + t1;
                B hfsq = B(0.5) * f * f;
                B dk = to_float(k);
                B r = fma(dk, log_2hi<B>(), fma(s, (hfsq + R), dk * log_2lo<B>()) - hfsq + f);
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(a >= B(0.)), nan<B>(), zz);
            }
        };

        template <class B>
        struct log_kernel<B, double>
        {
            /* origin: boost/simd/arch/common/simd/function/log.hpp */
            /*
             * ====================================================
             * copyright 2016 NumScale SAS
             *
             * Distributed under the Boost Software License, Version 1.0.
             * (See copy at http://boost.org/LICENSE_1_0.txt)
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                using i_type = as_integer_t<B>;

                B x = a;
                i_type hx = bitwise_cast<i_type>(x) >> 32;
                i_type k = zero<i_type>();
                auto isnez = (a != B(0.));
#ifndef XSIMD_NO_DENORMALS
                auto test = (a < smallestposval<B>()) && isnez;
                if (any(test))
                {
                    k = select(bool_cast(test), k - i_type(54), k);
                    x = select(test, x * B(18014398509481984ull), x);
                }
#endif
                hx += 0x3ff00000 - 0x3fe6a09e;
                k += (hx >> 20) - 0x3ff;
                B dk = to_float(k);
                hx = (hx & i_type(0x000fffff)) + 0x3fe6a09e;
                x = bitwise_cast<B>(hx << 32 | (i_type(0xffffffff) & bitwise_cast<i_type>(x)));

                B f = --x;
                B hfsq = B(0.5) * f * f;
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;

                B t1 = w * horner<B,
                                  0x3fd999999997fa04ll,
                                  0x3fcc71c51d8e78afll,
                                  0x3fc39a09d078c69fll>(w);
                B t2 = z * horner<B,
                                  0x3fe5555555555593ll,
                                  0x3fd2492494229359ll,
                                  0x3fc7466496cb03dell,
                                  0x3fc2f112df3e5244ll>(w);
                B R = t2 + t1;
                B r = fma(dk, log_2hi<B>(), fma(s, (hfsq + R), dk * log_2lo<B>()) - hfsq + f);
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(a >= B(0.)), nan<B>(), zz);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> log(const simd_base<B>& x)
    {
        return detail::log_kernel<batch_type_t<B>>::compute(x());
    }

    /***********************
     * log2 implementation *
     ***********************/

    namespace detail
    {
        template <class B, class T = typename B::value_type>
        struct log2_kernel;

        template <class B>
        struct log2_kernel<B, float>
        {
            /* origin: FreeBSD /usr/src/lib/msun/src/e_log2f.c */
            /*
             * ====================================================
             * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
             *
             * Developed at SunPro, a Sun Microsystems, Inc. business.
             * Permission to use, copy, modify, and distribute this
             * software is freely granted, provided that this notice
             * is preserved.
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                using i_type = as_integer_t<B>;
                B x = a;
                i_type k(0);
                auto isnez = (a != B(0.));
#ifndef XSIMD_NO_DENORMALS
                auto test = (a < smallestposval<B>()) && isnez;
                if (any(test))
                {
                    k = select(bool_cast(test), k - i_type(25), k);
                    x = select(test, x * B(33554432ul), x);
                }
#endif
                i_type ix = bitwise_cast<i_type>(x);
                ix += 0x3f800000 - 0x3f3504f3;
                k += (ix >> 23) - 0x7f;
                ix = (ix & i_type(0x007fffff)) + 0x3f3504f3;
                x = bitwise_cast<B>(ix);
                B f = --x;
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B, 0x3eccce13, 0x3e789e26>(w);
                B t2 = z * horner<B, 0x3f2aaaaa, 0x3e91e9ee>(w);
                B R = t1 + t2;
                B hfsq = B(0.5) * f * f;
                B dk = to_float(k);
                B r = fma(fms(s, hfsq + R, hfsq) + f, invlog_2<B>(), dk);
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(a >= B(0.)), nan<B>(), zz);
            }
        };

        template <class B>
        struct log2_kernel<B, double>
        {
            /* origin: FreeBSD /usr/src/lib/msun/src/e_log2f.c */
            /*
             * ====================================================
             * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
             *
             * Developed at SunPro, a Sun Microsystems, Inc. business.
             * Permission to use, copy, modify, and distribute this
             * software is freely granted, provided that this notice
             * is preserved.
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                using i_type = as_integer_t<B>;
                B x = a;
                i_type hx = bitwise_cast<i_type>(x) >> 32;
                i_type k = zero<i_type>();
                auto isnez = (a != B(0.));
#ifndef XSIMD_NO_DENORMALS
                auto test = (a < smallestposval<B>()) && isnez;
                if (any(test))
                {
                    k = select(bool_cast(test), k - i_type(54), k);
                    x = select(test, x * B(18014398509481984ull), x);
                }
#endif
                hx += 0x3ff00000 - 0x3fe6a09e;
                k += (hx >> 20) - 0x3ff;
                hx = (hx & i_type(0x000fffff)) + 0x3fe6a09e;
                x = bitwise_cast<B>(hx << 32 | (i_type(0xffffffff) & bitwise_cast<i_type>(x)));
                B f = --x;
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B,
                                  0x3fd999999997fa04ll,
                                  0x3fcc71c51d8e78afll,
                                  0x3fc39a09d078c69fll>(w);
                B t2 = z * horner<B,
                                  0x3fe5555555555593ll,
                                  0x3fd2492494229359ll,
                                  0x3fc7466496cb03dell,
                                  0x3fc2f112df3e5244ll>(w);
                B R = t2 + t1;
                B hfsq = B(0.5) * f * f;
                B hi = f - hfsq;
                hi = hi & bitwise_cast<B>((allbits<i_type>() << 32));
                B lo = fma(s, hfsq + R, f - hi - hfsq);
                B val_hi = hi * invlog_2hi<B>();
                B val_lo = fma(lo + hi, invlog_2lo<B>(), lo * invlog_2hi<B>());
                B dk = to_float(k);
                B w1 = dk + val_hi;
                val_lo += (dk - w1) + val_hi;
                val_hi = w1;
                B r = val_lo + val_hi;
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(a >= B(0.)), nan<B>(), zz);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> log2(const simd_base<B>& x)
    {
        return detail::log2_kernel<batch_type_t<B>>::compute(x());
    }

    /************************
     * log10 implementation *
     ************************/

    namespace detail
    {
        template <class B, class T = typename B::value_type>
        struct log10_kernel;

        template <class B>
        struct log10_kernel<B, float>
        {
            /* origin: FreeBSD /usr/src/lib/msun/src/e_log10f.c */
            /*
             * ====================================================
             * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
             *
             * Developed at SunPro, a Sun Microsystems, Inc. business.
             * Permission to use, copy, modify, and distribute this
             * software is freely granted, provided that this notice
             * is preserved.
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                const B
                    ivln10hi(4.3432617188e-01f),
                    ivln10lo(-3.1689971365e-05f),
                    log10_2hi(3.0102920532e-01f),
                    log10_2lo(7.9034151668e-07f);
                using i_type = as_integer_t<B>;
                B x = a;
                i_type k(0);
                auto isnez = (a != B(0.));
#ifndef XSIMD_NO_DENORMALS
                auto test = (a < smallestposval<B>()) && isnez;
                if (any(test))
                {
                    k = select(bool_cast(test), k - i_type(25), k);
                    x = select(test, x * B(33554432ul), x);
                }
#endif
                i_type ix = bitwise_cast<i_type>(x);
                ix += 0x3f800000 - 0x3f3504f3;
                k += (ix >> 23) - 0x7f;
                ix = (ix & i_type(0x007fffff)) + 0x3f3504f3;
                x = bitwise_cast<B>(ix);
                B f = --x;
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B, 0x3eccce13, 0x3e789e26>(w);
                B t2 = z * horner<B, 0x3f2aaaaa, 0x3e91e9ee>(w);
                B R = t2 + t1;
                B dk = to_float(k);
                B hfsq = B(0.5) * f * f;
                B hibits = f - hfsq;
                hibits &= bitwise_cast<B>(i_type(0xfffff000));
                B lobits = fma(s, hfsq + R, f - hibits - hfsq);
                B r = fma(dk, log10_2hi,
                          fma(hibits, ivln10hi,
                              fma(lobits, ivln10hi,
                                  fma(lobits + hibits, ivln10lo, dk * log10_2lo))));
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(a >= B(0.)), nan<B>(), zz);
            }
        };

        template <class B>
        struct log10_kernel<B, double>
        {
            /* origin: FreeBSD /usr/src/lib/msun/src/e_log10f.c */
            /*
             * ====================================================
             * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
             *
             * Developed at SunPro, a Sun Microsystems, Inc. business.
             * Permission to use, copy, modify, and distribute this
             * software is freely granted, provided that this notice
             * is preserved.
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                const B
                    ivln10hi(4.34294481878168880939e-01),
                    ivln10lo(2.50829467116452752298e-11),
                    log10_2hi(3.01029995663611771306e-01),
                    log10_2lo(3.69423907715893078616e-13);
                using i_type = as_integer_t<B>;
                B x = a;
                i_type hx = bitwise_cast<i_type>(x) >> 32;
                i_type k = zero<i_type>();
                auto isnez = (a != B(0.));
#ifndef XSIMD_NO_DENORMALS
                auto test = (a < smallestposval<B>()) && isnez;
                if (any(test))
                {
                    k = select(bool_cast(test), k - i_type(54), k);
                    x = select(test, x * B(18014398509481984ull), x);
                }
#endif
                hx += 0x3ff00000 - 0x3fe6a09e;
                k += (hx >> 20) - 0x3ff;
                hx = (hx & i_type(0x000fffff)) + 0x3fe6a09e;
                x = bitwise_cast<B>(hx << 32 | (i_type(0xffffffff) & bitwise_cast<i_type>(x)));
                B f = --x;
                B dk = to_float(k);
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B,
                                  0x3fd999999997fa04ll,
                                  0x3fcc71c51d8e78afll,
                                  0x3fc39a09d078c69fll>(w);
                B t2 = z * horner<B,
                                  0x3fe5555555555593ll,
                                  0x3fd2492494229359ll,
                                  0x3fc7466496cb03dell,
                                  0x3fc2f112df3e5244ll>(w);
                B R = t2 + t1;
                B hfsq = B(0.5) * f * f;
                B hi = f - hfsq;
                hi = hi & bitwise_cast<B>(allbits<i_type>() << 32);
                B lo = f - hi - hfsq + s * (hfsq + R);
                B val_hi = hi * ivln10hi;
                B y = dk * log10_2hi;
                B val_lo = dk * log10_2lo + (lo + hi) * ivln10lo + lo * ivln10hi;
                B w1 = y + val_hi;
                val_lo += (y - w1) + val_hi;
                val_hi = w1;
                B r = val_lo + val_hi;
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(a >= B(0.)), nan<B>(), zz);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> log10(const simd_base<B>& x)
    {
        return detail::log10_kernel<batch_type_t<B>>::compute(x());
    }

    /************************
     * log1p implementation *
     ************************/

    namespace detail
    {
        template <class B, class T = typename B::value_type>
        struct log1p_kernel;

        template <class B>
        struct log1p_kernel<B, float>
        {
            /* origin: boost/simd/arch/common/simd/function/log1p.hpp */
            /*
             * ====================================================
             * copyright 2016 NumScale SAS
             *
             * Distributed under the Boost Software License, Version 1.0.
             * (See copy at http://boost.org/LICENSE_1_0.txt)
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                using i_type = as_integer_t<B>;
                const B uf = a + B(1.);
                auto isnez = (uf != B(0.));
                i_type iu = bitwise_cast<i_type>(uf);
                iu += 0x3f800000 - 0x3f3504f3;
                i_type k = (iu >> 23) - 0x7f;
                iu = (iu & i_type(0x007fffff)) + 0x3f3504f3;
                B f = --(bitwise_cast<B>(iu));
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B, 0x3eccce13, 0x3e789e26>(w);
                B t2 = z * horner<B, 0x3f2aaaaa, 0x3e91e9ee>(w);
                B R = t2 + t1;
                B hfsq = B(0.5) * f * f;
                B dk = to_float(k);
                /* correction term ~ log(1+x)-log(u), avoid underflow in c/u */
                B c = select(bool_cast(k >= i_type(2)), B(1.) - (uf - a), a - (uf - B(1.))) / uf;
                B r = fma(dk, log_2hi<B>(), fma(s, (hfsq + R), dk * log_2lo<B>() + c) - hfsq + f);
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(uf >= B(0.)), nan<B>(), zz);
            }
        };

        template <class B>
        struct log1p_kernel<B, double>
        {
            /* origin: boost/simd/arch/common/simd/function/log1p.hpp */
            /*
             * ====================================================
             * copyright 2016 NumScale SAS
             *
             * Distributed under the Boost Software License, Version 1.0.
             * (See copy at http://boost.org/LICENSE_1_0.txt)
             * ====================================================
             */
            static inline B compute(const B& a)
            {
                using i_type = as_integer_t<B>;
                const B uf = a + B(1.);
                auto isnez = (uf != B(0.));
                i_type hu = bitwise_cast<i_type>(uf) >> 32;
                hu += 0x3ff00000 - 0x3fe6a09e;
                i_type k = (hu >> 20) - 0x3ff;
                /* correction term ~ log(1+x)-log(u), avoid underflow in c/u */
                B c = select(bool_cast(k >= i_type(2)), B(1.) - (uf - a), a - (uf - B(1.))) / uf;
                hu = (hu & i_type(0x000fffff)) + 0x3fe6a09e;
                B f = bitwise_cast<B>((hu << 32) | (i_type(0xffffffff) & bitwise_cast<i_type>(uf)));
                f = --f;
                B hfsq = B(0.5) * f * f;
                B s = f / (B(2.) + f);
                B z = s * s;
                B w = z * z;
                B t1 = w * horner<B,
                                  0x3fd999999997fa04ll,
                                  0x3fcc71c51d8e78afll,
                                  0x3fc39a09d078c69fll>(w);
                B t2 = z * horner<B,
                                  0x3fe5555555555593ll,
                                  0x3fd2492494229359ll,
                                  0x3fc7466496cb03dell,
                                  0x3fc2f112df3e5244ll>(w);
                B R = t2 + t1;
                B dk = to_float(k);
                B r = fma(dk, log_2hi<B>(), fma(s, hfsq + R, dk * log_2lo<B>() + c) - hfsq + f);
#ifndef XSIMD_NO_INFINITIES
                B zz = select(isnez, select(a == infinity<B>(), infinity<B>(), r), minusinfinity<B>());
#else
                B zz = select(isnez, r, minusinfinity<B>());
#endif
                return select(!(uf >= B(0.)), nan<B>(), zz);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> log1p(const simd_base<B>& x)
    {
        return detail::log1p_kernel<batch_type_t<B>>::compute(x());
    }
}

#endif
