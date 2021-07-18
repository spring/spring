/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_INVTRIGO_HPP
#define XSIMD_INVTRIGO_HPP

#include "xsimd_fp_sign.hpp"
#include "xsimd_horner.hpp"
#include "xsimd_numerical_constant.hpp"

namespace xsimd
{
    namespace detail
    {
        template <class B, class T = typename B::value_type>
        struct invtrigo_kernel_impl;

        /* origin: boost/simd/arch/common/detail/simd/f_invtrig.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */
        template <class B>
        struct invtrigo_kernel_impl<B, float>
        {
            static inline B asin(const B& a)
            {
                B x = abs(a);
                B sign = bitofsign(a);
                auto x_larger_05 = x > B(0.5);
                B z = select(x_larger_05, B(0.5) * (B(1.) - x), x * x);
                x = select(x_larger_05, sqrt(z), x);
                B z1 = horner<B,
                              0x3e2aaae4,
                              0x3d9980f6,
                              0x3d3a3ec7,
                              0x3cc617e3,
                              0x3d2cb352>(z);
                z1 = fma(z1, z * x, x);
                z = select(x_larger_05, pio2<B>() - (z1 + z1), z1);
                return z ^ sign;
            }

            static inline B kernel_atan(const B& x, const B& recx)
            {
                const auto flag1 = x < tan3pio8<B>();
                const auto flag2 = (x >= B(detail::caster32_t(0x3ed413cd).f)) && flag1;
                B yy = select(flag1, B(0.), pio2<B>());
                yy = select(flag2, pio4<B>(), yy);
                B xx = select(flag1, x, -recx);
                xx = select(flag2, (x - B(1.)) / (x + B(1.)), xx);
                const B z = xx * xx;
                B z1 = horner<B,
                              0xbeaaaa2aul,
                              0x3e4c925ful,
                              0xbe0e1b85ul,
                              0x3da4f0d1ul>(z);
                z1 = fma(xx, z1 * z, xx);
                z1 = select(flag2, z1 + pio_4lo<B>(), z1);
                z1 = select(!flag1, z1 + pio_2lo<B>(), z1);
                return yy + z1;
            }
        };

        /* origin: boost/simd/arch/common/detail/simd/d_invtrig.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */
        template <class B>
        struct invtrigo_kernel_impl<B, double>
        {
            static inline B asin(const B& a)
            {
                B x = abs(a);
                auto small_cond = x < sqrteps<B>();
                B ct1 = B(detail::caster64_t(int64_t(0x3fe4000000000000)).f);
                B zz1 = B(1.) - x;
                B vp = zz1 * horner<B,
                                    0x403c896240f3081dull,
                                    0xc03991aaac01ab68ull,
                                    0x401bdff5baf33e6aull,
                                    0xbfe2079259f9290full,
                                    0x3f684fc3988e9f08ull>(zz1) /
                    horner1<B,
                            0x40756709b0b644beull,
                            0xc077fe08959063eeull,
                            0x40626219af6a7f42ull,
                            0xc035f2a2b6bf5d8cull>(zz1);
                zz1 = sqrt(zz1 + zz1);
                B z = pio4<B>() - zz1;
                zz1 = fms(zz1, vp, pio_2lo<B>());
                z = z - zz1;
                zz1 = z + pio4<B>();
                B zz2 = a * a;
                z = zz2 * horner<B,
                                 0xc020656c06ceafd5ull,
                                 0x40339007da779259ull,
                                 0xc0304331de27907bull,
                                 0x4015c74b178a2dd9ull,
                                 0xbfe34341333e5c16ull,
                                 0x3f716b9b0bd48ad3ull>(zz2) /
                    horner1<B,
                            0xc04898220a3607acull,
                            0x4061705684ffbf9dull,
                            0xc06265bb6d3576d7ull,
                            0x40519fc025fe9054ull,
                            0xc02d7b590b5e0eabull>(zz2);
                zz2 = fma(x, z, x);
                return select(x > B(1.), nan<B>(),
                              select(small_cond, x,
                                     select(x > ct1, zz1, zz2)) ^
                                  bitofsign(a));
            }

            static inline B kernel_atan(const B& x, const B& recx)
            {
                const auto flag1 = x < tan3pio8<B>();
                const auto flag2 = (x >= tanpio8<B>()) && flag1;
                B yy = select(flag1, B(0.), pio2<B>());
                yy = select(flag2, pio4<B>(), yy);
                B xx = select(flag1, x, -recx);
                xx = select(flag2, (x - B(1.)) / (x + B(1.)), xx);
                B z = xx * xx;
                z *= horner<B,
                            0xc0503669fd28ec8eull,
                            0xc05eb8bf2d05ba25ull,
                            0xc052c08c36880273ull,
                            0xc03028545b6b807aull,
                            0xbfec007fa1f72594ull>(z) /
                    horner1<B,
                            0x4068519efbbd62ecull,
                            0x407e563f13b049eaull,
                            0x407b0e18d2e2be3bull,
                            0x4064a0dd43b8fa25ull,
                            0x4038dbc45b14603cull>(z);
                z = fma(xx, z, xx);
                z = select(flag2, z + pio_4lo<B>(), z);
                z = z + select(flag1, B(0.), pio_2lo<B>());
                return yy + z;
            }
        };

        template <class B>
        struct invtrigo_kernel
        {
            static inline B asin(const B& a)
            {
                return invtrigo_kernel_impl<B>::asin(a);
            }

            static inline B acos(const B& a)
            {
                B x = abs(a);
                auto x_larger_05 = x > B(0.5);
                x = select(x_larger_05, sqrt(fma(B(-0.5), x, B(0.5))), a);
                x = asin(x);
                x = select(x_larger_05, x + x, x);
                x = select(a < B(-0.5), pi<B>() - x, x);
                return select(x_larger_05, x, pio2<B>() - x);
            }

            static inline B atan(const B& a)
            {
                const B absa = abs(a);
                const B x = kernel_atan(absa, B(1.) / absa);
                return x ^ bitofsign(a);
            }

            static inline B acot(const B& a)
            {
                const B absa = abs(a);
                const B x = kernel_atan(B(1.) / absa, absa);
                return x ^ bitofsign(a);
            }

            static inline B atan2(const B& y, const B& x)
            {
                const B q = abs(y / x);
                const B z = kernel_atan(q, B(1.) / q);
                return select(x > B(0.), z, pi<B>() - z) * signnz(y);
            }

            static inline B kernel_atan(const B& x, const B& recx)
            {
                return invtrigo_kernel_impl<B>::kernel_atan(x, recx);
            }
        };
    }
}

#endif
