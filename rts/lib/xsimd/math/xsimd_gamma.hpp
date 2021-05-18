/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_GAMMA_HPP
#define XSIMD_GAMMA_HPP

#include "xsimd_basic_math.hpp"
#include "xsimd_exponential.hpp"
#include "xsimd_horner.hpp"
#include "xsimd_logarithm.hpp"
#include "xsimd_trigonometric.hpp"

namespace xsimd
{
    /**
     * Computes the gamma function of the batch \c x.
     * @param x batch of floating point values.
     * @return the gamma function of \c x.
     */
    template <class B>
    batch_type_t<B> tgamma(const simd_base<B>& x);

    /**
     * Computes the natural logarithm of the gamma function of the batch \c x.
     * @param x batch of floating point values.
     * @return the natural logarithm of the gamma function of \c x.
     */
    template <class B>
    batch_type_t<B> lgamma(const simd_base<B>& x);

    /*************************
     * tgamma implementation *
     *************************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/detail/generic/stirling_kernel.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B, class T = typename B::value_type>
        struct stirling_kernel;

        template <class B>
        struct stirling_kernel<B, float>
        {
            static inline B compute(const B& x)
            {
                return horner<B,
                              0x3daaaaab,
                              0x3b638e39,
                              0xbb2fb930,
                              0xb970b359>(x);
            }

            static inline B split_limit()
            {
                return B(detail::caster32_t(uint32_t(0x41d628f6)).f);
            }

            static inline B large_limit()
            {
                return B(detail::caster32_t(uint32_t(0x420c28f3)).f);
            }
        };

        template <class B>
        struct stirling_kernel<B, double>
        {
            static inline B compute(const B& x)
            {
                return horner<B,
                              0x3fb5555555555986ull,  //   8.33333333333482257126E-2
                              0x3f6c71c71b98c5fdull,  //   3.47222221605458667310E-3
                              0xbf65f72607d44fd7ull,  //  -2.68132617805781232825E-3
                              0xbf2e166b27e61d7cull,  //  -2.29549961613378126380E-4
                              0x3f49cc72592d7293ull   //   7.87311395793093628397E-4
                              >(x);
            }

            static inline B split_limit()
            {
                return B(detail::caster64_t(uint64_t(0x4061e083ba3443d4)).f);
            }

            static inline B large_limit()
            {
                return B(detail::caster64_t(uint64_t(0x4065800000000000)).f);
            }
        };

        /* origin: boost/simd/arch/common/simd/function/stirling.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B>
        inline B stirling(const B& a)
        {
            const B stirlingsplitlim = stirling_kernel<B>::split_limit();
            const B stirlinglargelim = stirling_kernel<B>::large_limit();
            B x = select(a >= B(0.), a, nan<B>());
            B w = B(1.) / x;
            w = fma(w, stirling_kernel<B>::compute(w), B(1.));
            B y = exp(-x);
            auto test = (x < stirlingsplitlim);
            B z = x - B(0.5);
            z = select(test, z, B(0.5) * z);
            B v = exp(z * log(abs(x)));
            y *= v;
            y = select(test, y, y * v);
            y *= sqrt_2pi<B>() * w;
#ifndef XSIMD_NO_INFINITIES
            y = select(xsimd::isinf(x), x, y);
#endif
            return select(x > stirlinglargelim, infinity<B>(), y);
        }
    }

    /* origin: boost/simd/arch/common/detail/generic/gamma_kernel.hpp */
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
        template <class B, class T = typename B::value_type>
        struct tgamma_kernel;

        template <class B>
        struct tgamma_kernel<B, float>
        {
            static inline B compute(const B& x)
            {
                return horner<B,
                              0x3f800000UL,  //  9.999999757445841E-01
                              0x3ed87799UL,  //  4.227874605370421E-01
                              0x3ed2d411UL,  //  4.117741948434743E-01
                              0x3da82a34UL,  //  8.211174403261340E-02
                              0x3d93ae7cUL,  //  7.211014349068177E-02
                              0x3b91db14UL,  //  4.451165155708328E-03
                              0x3ba90c99UL,  //  5.158972571345137E-03
                              0x3ad28b22UL   //  1.606319369134976E-03
                              >(x);
            }
        };

        template <class B>
        struct tgamma_kernel<B, double>
        {
            static inline B compute(const B& x)
            {
                return horner<B,
                              0x3ff0000000000000ULL,  // 9.99999999999999996796E-1
                              0x3fdfa1373993e312ULL,  // 4.94214826801497100753E-1
                              0x3fca8da9dcae7d31ULL,  // 2.07448227648435975150E-1
                              0x3fa863d918c423d3ULL,  // 4.76367800457137231464E-2
                              0x3f8557cde9db14b0ULL,  // 1.04213797561761569935E-2
                              0x3f5384e3e686bfabULL,  // 1.19135147006586384913E-3
                              0x3f24fcb839982153ULL   // 1.60119522476751861407E-4
                              >(x) /
                    horner<B,
                           0x3ff0000000000000ULL,  //  1.00000000000000000320E00
                           0x3fb24944c9cd3c51ULL,  //  7.14304917030273074085E-2
                           0xbfce071a9d4287c2ULL,  // -2.34591795718243348568E-1
                           0x3fa25779e33fde67ULL,  //  3.58236398605498653373E-2
                           0x3f8831ed5b1bb117ULL,  //  1.18139785222060435552E-2
                           0xBf7240e4e750b44aULL,  // -4.45641913851797240494E-3
                           0x3f41ae8a29152573ULL,  //  5.39605580493303397842E-4
                           0xbef8487a8400d3aFULL   // -2.31581873324120129819E-5
                           >(x);
            }
        };
    }

    /* origin: boost/simd/arch/common/simd/function/gamma.hpp */
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
        B tgamma_large_negative(const B& a)
        {
            B st = stirling(a);
            B p = floor(a);
            B sgngam = select(is_even(p), -B(1.), B(1.));
            B z = a - p;
            auto test2 = z < B(0.5);
            z = select(test2, z - B(1.), z);
            z = a * trigo_kernel<B>::sin(z, trigo_pi_tag());
            z = abs(z);
            return sgngam * pi<B>() / (z * st);
        }

        template <class B, class BB>
        B tgamma_other(const B& a, const BB& test)
        {
            B x = select(test, B(2.), a);
#ifndef XSIMD_NO_INFINITIES
            auto inf_result = (a == infinity<B>());
            x = select(inf_result, B(2.), x);
#endif
            B z = B(1.);
            auto test1 = (x >= B(3.));
            while (any(test1))
            {
                x = select(test1, x - B(1.), x);
                z = select(test1, z * x, z);
                test1 = (x >= B(3.));
            }
            test1 = (x < B(0.));
            while (any(test1))
            {
                z = select(test1, z / x, z);
                x = select(test1, x + B(1.), x);
                test1 = (x < B(0.));
            }
            auto test2 = (x < B(2.));
            while (any(test2))
            {
                z = select(test2, z / x, z);
                x = select(test2, x + B(1.), x);
                test2 = (x < B(2.));
            }
            x = z * tgamma_kernel<B>::compute(x - B(2.));
#ifndef XSIMD_NO_INFINITIES
            return select(inf_result, a, x);
#else
            return x;
#endif
        }

        template <class B>
        inline B tgamma_impl(const B& a)
        {
            auto nan_result = (a < B(0.) && is_flint(a));
#ifndef XSIMD_NO_INVALIDS
            nan_result = xsimd::isnan(a) || nan_result;
#endif
            B q = abs(a);
            auto test = (a < B(-33.));
            B r = nan<B>();
            if (any(test))
            {
                r = tgamma_large_negative(q);
                if (all(test))
                    return select(nan_result, nan<B>(), r);
            }
            B r1 = tgamma_other(a, test);
            B r2 = select(test, r, r1);
            return select(a == B(0.), copysign(infinity<B>(), a), select(nan_result, nan<B>(), r2));
        }
    }

    template <class B>
    inline batch_type_t<B> tgamma(const simd_base<B>& x)
    {
        return detail::tgamma_impl(x());
    }

    /*************************
     * lgamma implementation *
     *************************/

    /* origin: boost/simd/arch/common/detail/generic/gammaln_kernel.hpp */
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
        template <class B, class T = typename B::value_type>
        struct lgamma_kernel;

        template <class B>
        struct lgamma_kernel<B, float>
        {
            static inline B gammalnB(const B& x)
            {
                return horner<B,
                              0x3ed87730,  //    4.227843421859038E-001
                              0x3ea51a64,  //    3.224669577325661E-001,
                              0xbd89f07e,  //   -6.735323259371034E-002,
                              0x3ca89ed8,  //    2.058355474821512E-002,
                              0xbbf164fd,  //   -7.366775108654962E-003,
                              0x3b3ba883,  //    2.863437556468661E-003,
                              0xbaabeab1,  //   -1.311620815545743E-003,
                              0x3a1ebb94  //    6.055172732649237E-004
                              >(x);
            }

            static inline B gammalnC(const B& x)
            {
                return horner<B,
                              0xbf13c468,  //   -5.772156501719101E-001
                              0x3f528d34,  //    8.224670749082976E-001,
                              0xbecd27a8,  //   -4.006931650563372E-001,
                              0x3e8a898b,  //    2.705806208275915E-001,
                              0xbe53c04f,  //   -2.067882815621965E-001,
                              0x3e2d4dab,  //    1.692415923504637E-001,
                              0xbe22d329,  //   -1.590086327657347E-001,
                              0x3e0c3c4f  //    1.369488127325832E-001
                              >(x);
            }

            static inline B gammaln2(const B& x)
            {
                return horner<B,
                              0x3daaaa94,  //   8.333316229807355E-002f
                              0xbb358701,  //  -2.769887652139868E-003f,
                              0x3a31fd69  //   6.789774945028216E-004f
                              >(x);
            }
        };

        template <class B>
        struct lgamma_kernel<B, double>
        {
            static inline B gammaln1(const B& x)
            {
                return horner<B,
                              0xc12a0c675418055eull,  //  -8.53555664245765465627E5
                              0xc13a45890219f20bull,  //  -1.72173700820839662146E6,
                              0xc131bc82f994db51ull,  //  -1.16237097492762307383E6,
                              0xc1143d73f89089e5ull,  //  -3.31612992738871184744E5,
                              0xc0e2f234355bb93eull,  //  -3.88016315134637840924E4,
                              0xc09589018ff36761ull  //  -1.37825152569120859100E3
                              >(x) /
                    horner<B,
                           0xc13ece4b6a11e14aull,  //  -2.01889141433532773231E6
                           0xc1435255892ff34cull,  //  -2.53252307177582951285E6,
                           0xc131628671950043ull,  //  -1.13933444367982507207E6,
                           0xc10aeb84b9744c9bull,  //  -2.20528590553854454839E5,
                           0xc0d0aa0d7b89d757ull,  //  -1.70642106651881159223E4,
                           0xc075fd0d1cf312b2ull,  //  -3.51815701436523470549E2,
                           0x3ff0000000000000ull  //   1.00000000000000000000E0
                           >(x);
            }

            static inline B gammalnA(const B& x)
            {
                return horner<B,
                              0x3fb555555555554bull,  //    8.33333333333331927722E-2
                              0xbf66c16c16b0a5a1ull,  //   -2.77777777730099687205E-3,
                              0x3f4a019f20dc5ebbull,  //    7.93650340457716943945E-4,
                              0xbf437fbdb580e943ull,  //   -5.95061904284301438324E-4,
                              0x3f4a985027336661ull  //    8.11614167470508450300E-4
                              >(x);
            }
        };
    }

    /* origin: boost/simd/arch/common/simd/function/gammaln.hpp */
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
        template <class B, class T = typename B::value_type>
        struct lgamma_impl;

        template <class B>
        struct lgamma_impl<B, float>
        {
            static inline B compute(const B& a)
            {
                auto inf_result = (a <= B(0.)) && is_flint(a);
                B x = select(inf_result, nan<B>(), a);
                B q = abs(x);
#ifndef XSIMD_NO_INFINITIES
                inf_result = (x == infinity<B>()) || inf_result;
#endif
                auto ltza = a < B(0.);
                B r;
                B r1 = other(q);
                if (any(ltza))
                {
                    r = select(inf_result, infinity<B>(), negative(q, r1));
                    if (all(ltza))
                        return r;
                }
                B r2 = select(ltza, r, r1);
                return select(a == minusinfinity<B>(), nan<B>(), select(inf_result, infinity<B>(), r2));
            }

        private:

            static inline B negative(const B& q, const B& w)
            {
                B p = floor(q);
                B z = q - p;
                auto test2 = z < B(0.5);
                z = select(test2, z - B(1.), z);
                z = q * trigo_kernel<B>::sin(z, trigo_pi_tag());
                return -log(invpi<B>() * abs(z)) - w;
            }

            static inline B other(const B& x)
            {
                auto xlt650 = (x < B(6.5));
                B r0x = x;
                B r0z = x;
                B r0s = B(1.);
                B r1 = B(0.);
                B p = nan<B>();
                if (any(xlt650))
                {
                    B z = B(1.);
                    B tx = select(xlt650, x, B(0.));
                    B nx = B(0.);
                    const B _075 = B(0.75);
                    const B _150 = B(1.50);
                    const B _125 = B(1.25);
                    const B _250 = B(2.50);
                    auto xge150 = (x >= _150);
                    auto txgt250 = (tx > _250);

                    // x >= 1.5
                    while (any(xge150 && txgt250))
                    {
                        nx = select(txgt250, nx - B(1.), nx);
                        tx = select(txgt250, x + nx, tx);
                        z = select(txgt250, z * tx, z);
                        txgt250 = (tx > _250);
                    }
                    r0x = select(xge150, x + nx - B(2.), x);
                    r0z = select(xge150, z, r0z);
                    r0s = select(xge150, B(1.), r0s);

                    // x >= 1.25 && x < 1.5
                    auto xge125 = (x >= _125);
                    auto xge125t = xge125 && !xge150;
                    if (any(xge125))
                    {
                        r0x = select(xge125t, x - B(1.), r0x);
                        r0z = select(xge125t, z * x, r0z);
                        r0s = select(xge125t, B(-1.), r0s);
                    }

                    // x >= 0.75 && x < 1.5
                    auto kernelC = as_logical_t<B>(false);
                    auto xge075 = (x >= _075);
                    auto xge075t = xge075 && !xge125;
                    if (any(xge075t))
                    {
                        kernelC = xge075t;
                        r0x = select(xge075t, x - B(1.), x);
                        r0z = select(xge075t, B(1.), r0z);
                        r0s = select(xge075t, B(-1.), r0s);
                        p = lgamma_kernel<B>::gammalnC(r0x);
                    }

                    // tx < 1.5 && x < 0.75
                    auto txlt150 = (tx < _150) && !xge075;
                    if (any(txlt150))
                    {
                        auto orig = txlt150;
                        while (any(txlt150))
                        {
                            z = select(txlt150, z * tx, z);
                            nx = select(txlt150, nx + B(1.), nx);
                            tx = select(txlt150, x + nx, tx);
                            txlt150 = (tx < _150) && !xge075;
                        }
                        r0x = select(orig, r0x + nx - B(2.), r0x);
                        r0z = select(orig, z, r0z);
                        r0s = select(orig, B(-1.), r0s);
                    }
                    p = select(kernelC, p, lgamma_kernel<B>::gammalnB(r0x));
                    if (all(xlt650))
                        return fma(r0x, p, r0s * log(abs(r0z)));
                }
                r0z = select(xlt650, abs(r0z), x);
                B m = log(r0z);
                r1 = fma(r0x, p, r0s * m);
                B r2 = fma(x - B(0.5), m, logsqrt2pi<B>() - x);
                r2 += lgamma_kernel<B>::gammaln2(B(1.) / (x * x)) / x;
                return select(xlt650, r1, r2);
            }
        };

        template <class B>
        struct lgamma_impl<B, double>
        {

            static inline B compute(const B& a)
            {
                auto inf_result = (a <= B(0.)) && is_flint(a);
                B x = select(inf_result, nan<B>(), a);
                B q = abs(x);
#ifndef XSIMD_NO_INFINITIES
                inf_result = (q == infinity<B>());
#endif
                auto test = (a < B(-34.));
                B r = nan<B>();
                if (any(test))
                {
                    r = large_negative(q);
                    if (all(test))
                        return select(inf_result, nan<B>(), r);
                }
                B r1 = other(a);
                B r2 = select(test, r, r1);
                return select(a == minusinfinity<B>(), nan<B>(), select(inf_result, infinity<B>(), r2));
            }

        private:

            static inline B large_negative(const B& q)
            {
                B w = lgamma(q);
                B p = floor(q);
                B z = q - p;
                auto test2 = (z < B(0.5));
                z = select(test2, z - B(1.), z);
                z = q * trigo_kernel<B>::sin(z, trigo_pi_tag());
                z = abs(z);
                return logpi<B>() - log(z) - w;
            }

            static inline B other(const B& xx)
            {
                B x = xx;
                auto test = (x < B(13.));
                B r1 = B(0.);
                if (any(test))
                {
                    B z = B(1.);
                    B p = B(0.);
                    B u = select(test, x, B(0.));
                    auto test1 = (u >= B(3.));
                    while (any(test1))
                    {
                        p = select(test1, p - B(1.), p);
                        u = select(test1, x + p, u);
                        z = select(test1, z * u, z);
                        test1 = (u >= B(3.));
                    }

                    auto test2 = (u < B(2.));
                    while (any(test2))
                    {
                        z = select(test2, z / u, z);
                        p = select(test2, p + B(1.), p);
                        u = select(test2, x + p, u);
                        test2 = (u < B(2.));
                    }

                    z = abs(z);
                    x += p - B(2.);
                    r1 = x * lgamma_kernel<B>::gammaln1(x) + log(z);
                    if (all(test))
                        return r1;
                }
                B r2 = fma(xx - B(0.5), log(xx), logsqrt2pi<B>() - xx);
                B p = B(1.) / (xx * xx);
                r2 += lgamma_kernel<B>::gammalnA(p) / xx;
                return select(test, r1, r2);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> lgamma(const simd_base<B>& x)
    {
        return detail::lgamma_impl<batch_type_t<B>>::compute(x());
    }
}

#endif
