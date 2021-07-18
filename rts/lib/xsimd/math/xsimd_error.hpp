/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ERROR_HPP
#define XSIMD_ERROR_HPP

#include "xsimd_basic_math.hpp"
#include "xsimd_exponential.hpp"
#include "xsimd_fp_sign.hpp"
#include "xsimd_horner.hpp"

namespace xsimd
{
    /**
     * Computes the error function of the batch \c x.
     * @param x batch of floating point values.
     * @return the error function of \c x.
     */
    template <class B>
    batch_type_t<B> erf(const simd_base<B>& x);

    /**
     * Computes the complementary error function of the batch \c x.
     * @param x batch of floating point values.
     * @return the error function of \c x.
     */
    template <class B>
    batch_type_t<B> erfc(const simd_base<B>& x);

    /**********************
     * erf implementation *
     **********************/

    namespace detail
    {
        /* origin: boost/simd/arch/common/detail/generic/erf_kernel.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */
        template <class B, class T = typename B::value_type>
        struct erf_kernel;

        template <class B>
        struct erf_kernel<B, float>
        {
            // computes erf(a0)/a0
            // x is sqr(a0) and 0 <= abs(a0) <= 2/3
            static inline B erf1(const B& x)
            {
                return horner<B,
                              0x3f906eba,  //   1.128379154774254e+00
                              0xbec0937e,  //  -3.761252839094832e-01
                              0x3de70f22,  //   1.128218315189123e-01
                              0xbcdb61f4,  //  -2.678010670585737e-02
                              0x3ba4468d,  //   5.013293006147870e-03
                              0xba1fc83b  //  -6.095205117313012e-04
                              >(x);
            }

            // computes erfc(x)*exp(sqr(x))
            // x >=  2/3
            static inline B erfc2(const B& x)
            {
                return horner<B,
                              0x3f0a0e8b,  //   5.392844046572836e-01
                              0xbf918a62,  //  -1.137035586823118e+00
                              0x3e243828,  //   1.603704761054187e-01
                              0x3ec4ca6e,  //   3.843569094305250e-01
                              0x3e1175c7,  //   1.420508523645926e-01
                              0x3e2006f0,  //   1.562764709849380e-01
                              0xbfaea865,  //  -1.364514006347145e+00
                              0x4050b063,  //   3.260765682222576e+00
                              0xc0cd1a85,  //  -6.409487379234005e+00
                              0x40d67e3b,  //   6.702908785399893e+00
                              0xc0283611  //  -2.628299919293280e+00
                              >(x);
            }

            static inline B erfc3(const B& x)
            {
                return (B(1.) - x) * horner<B,
                                            0x3f7ffffe,  //   9.9999988e-01
                                            0xbe036d7e,  //  -1.2834737e-01
                                            0xbfa11698,  //  -1.2585020e+00
                                            0xbffc9284,  //  -1.9732213e+00
                                            0xc016c985,  //  -2.3560498e+00
                                            0x3f2cff3b,  //   6.7576951e-01
                                            0xc010d956,  //  -2.2632651e+00
                                            0x401b5680,  //   2.4271545e+00
                                            0x41aa8e55  //   2.1319498e+01
                                            >(x);
            }
        };

        template <class B>
        struct erf_kernel<B, double>
        {
            // computes erf(a0)/a0
            // x is sqr(a0) and 0 <= abs(a0) <= 0.65
            static inline B erf1(const B& x)
            {
                return horner<B,
                              0x3ff20dd750429b61ull,  // 1.12837916709551
                              0x3fc16500f106c0a5ull,  // 0.135894887627278
                              0x3fa4a59a4f02579cull,  // 4.03259488531795E-02
                              0x3f53b7664358865aull,  // 1.20339380863079E-03
                              0x3f110512d5b20332ull  // 6.49254556481904E-05
                              >(x) /
                    horner<B,
                           0x3ff0000000000000ull,  // 1
                           0x3fdd0a84eb1ca867ull,  // 0.453767041780003
                           0x3fb64536ca92ea2full,  // 8.69936222615386E-02
                           0x3f8166f75999dbd1ull,  // 8.49717371168693E-03
                           0x3f37ea4332348252ull  // 3.64915280629351E-04
                           >(x);
            }

            // computes erfc(x)*exp(x*x)
            // 0.65 <= abs(x) <= 2.2
            static inline B erfc2(const B& x)
            {
                return horner<B,
                              0x3feffffffbbb552bull,  // 0.999999992049799
                              0x3ff54dfe9b258a60ull,  // 1.33154163936765
                              0x3fec1986509e687bull,  // 0.878115804155882
                              0x3fd53dd7a67c7e9full,  // 0.331899559578213
                              0x3fb2488a6b5cb5e5ull,  // 7.14193832506776E-02
                              0x3f7cf4cfe0aacbb4ull,  // 7.06940843763253E-03
                              0x0ull  // 0
                              >(x) /
                    horner<B,
                           0x3ff0000000000000ull,  // 1
                           0x4003adeae79b9708ull,  // 2.45992070144246
                           0x40053b1052dca8bdull,  // 2.65383972869776
                           0x3ff9e677c2777c3cull,  // 1.61876655543871
                           0x3fe307622fcff772ull,  // 0.594651311286482
                           0x3fc033c113a7deeeull,  // 0.126579413030178
                           0x3f89a996639b0d00ull  // 1.25304936549413E-02
                           >(x);
            }

            // computes erfc(x)*exp(x*x)
            // 2.2 <= abs(x) <= 6
            static inline B erfc3(const B& x)
            {
                return horner<B,
                              0x3fefff5a9e697ae2ull,  //0.99992114009714
                              0x3ff9fa202deb88e5ull,  //1.62356584489367
                              0x3ff44744306832aeull,  //1.26739901455873
                              0x3fe29be1cff90d94ull,  //0.581528574177741
                              0x3fc42210f88b9d43ull,  //0.157289620742839
                              0x3f971d0907ea7a92ull,  //2.25716982919218E-02
                              0x0ll  //0
                              >(x) /
                    horner<B,
                           0x3ff0000000000000ull,  //1
                           0x400602f24bf3fdb6ull,  //2.75143870676376
                           0x400afd487397568full,  //3.37367334657285
                           0x400315ffdfd5ce91ull,  //2.38574194785344
                           0x3ff0cfd4cb6cde9full,  //1.05074004614827
                           0x3fd1d7ab774bb837ull,  //0.278788439273629
                           0x3fa47bd61bbb3843ull  //4.00072964526861E-02
                           >(x);
            }

            // computes erfc(rx)*exp(rx*rx)
            // x >=  6 rx = 1/x
            static inline B erfc4(const B& x)
            {
                return horner<B,
                              0xbc7e4ad1ec7d0000ll,  // -2.627435221016534e-17
                              0x3fe20dd750429a16ll,  // 5.641895835477182e-01
                              0x3db60000e984b501ll,  // 2.000889609806154e-11
                              0xbfd20dd753ae5dfdll,  // -2.820947949598745e-01
                              0x3e907e71e046a820ll,  // 2.457786367990903e-07
                              0x3fdb1494cac06d39ll,  // 4.231311779019112e-01
                              0x3f34a451701654f1ll,  // 3.149699042180451e-04
                              0xbff105e6b8ef1a63ll,  // -1.063940737150596e+00
                              0x3fb505a857e9ccc8ll,  // 8.211757799454056e-02
                              0x40074fbabc514212ll,  // 2.913930388669777e+00
                              0x4015ac7631f7ac4fll,  // 5.418419628850713e+00
                              0xc0457e03041e9d8bll,  // -4.298446704382794e+01
                              0x4055803d26c4ec4fll,  // 8.600373238783617e+01
                              0xc0505fce04ec4ec5ll  // -6.549694941594051e+01
                              >(x);
            }
        };

        /* origin: boost/simd/arch/common/simd/function/erf.hpp */
        /*
         * ====================================================
         * copyright 2016 NumScale SAS
         *
         * Distributed under the Boost Software License, Version 1.0.
         * (See copy at http://boost.org/LICENSE_1_0.txt)
         * ====================================================
         */

        template <class B, class T = typename B::value_type>
        struct erf_impl;

        template <class B>
        struct erf_impl<B, float>
        {
            static inline B compute(const B& a)
            {
                B x = abs(a);
                B r1 = B(0.);
                auto test1 = x < B(2.f / 3.f);
                if (any(test1))
                {
                    r1 = a * erf_kernel<B>::erf1(x * x);
                    if (all(test1))
                        return r1;
                }
                B z = x / (B(1.) + x);
                z -= B(0.4f);
                B r2 = B(1.) - exp(-x * x) * erf_kernel<B>::erfc2(z);
                r2 = select(a < B(0.), -r2, r2);
                r1 = select(test1, r1, r2);
#ifndef XSIMD_NO_INFINITIES
                r1 = select(xsimd::isinf(a), sign(a), r1);
#endif
                return r1;
            }
        };

        template <class B>
        struct erf_impl<B, double>
        {
            static inline B compute(const B& a)
            {
                B x = abs(a);
                B xx = x * x;
                B lim1 = B(0.65);
                B lim2 = B(2.2);
                auto test1 = x < lim1;
                B r1 = B(0.);
                if (any(test1))
                {
                    r1 = a * erf_kernel<B>::erf1(xx);
                    if (all(test1))
                        return r1;
                }
                auto test2 = x < lim2;
                auto test3 = test2 && !test1;
                B ex = exp(-xx);
                if (any(test3))
                {
                    B z = B(1.) - ex * erf_kernel<B>::erfc2(x);
                    B r2 = select(a < B(0.), -z, z);
                    r1 = select(test1, r1, r2);
                    if (all(test1 || test3))
                        return r1;
                }
                B z = B(1.) - ex * erf_kernel<B>::erfc3(x);
                z = select(a < B(0.), -z, z);
#ifndef XSIMD_NO_INFINITIES
                z = select(xsimd::isinf(a), sign(a), z);
#endif
                return select(test2, r1, z);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> erf(const simd_base<B>& x)
    {
        return detail::erf_impl<batch_type_t<B>>::compute(x());
    }

    /***********************
     * erfc implementation *
     ***********************/

    namespace detail
    {
        template <class B, class T = typename B::value_type>
        struct erfc_impl;

        template <class B>
        struct erfc_impl<B, float>
        {
            static inline B compute(const B& a)
            {
                B x = abs(a);
                auto test0 = a < B(0.);
                B r1 = B(0.);
                auto test1 = x < B(2.f / 3.f);
                B z = x / (B(1.) + x);
                if (any(test1))
                {
                    r1 = erf_kernel<B>::erfc3(z);
                    if (all(test1))
                        return select(test0, B(2.) - r1, r1);
                }
                z -= B(0.4f);
                B r2 = exp(-x * x) * erf_kernel<B>::erfc2(z);
                r1 = select(test1, r1, r2);
#ifndef XSIMD_NO_INFINITIES
                r1 = select(x == infinity<B>(), B(0.), r1);
#endif
                return select(test0, B(2.) - r1, r1);
            }
        };

        template <class B>
        struct erfc_impl<B, double>
        {
            static inline B compute(const B& a)
            {
                B x = abs(a);
                B xx = x * x;
                B lim1 = B(0.65);
                B lim2 = B(2.2);
                auto test0 = a < B(0.);
                auto test1 = x < lim1;
                B r1 = B(0.);
                if (any(test1))
                {
                    r1 = B(1.) - x * erf_kernel<B>::erf1(xx);
                    if (all(test1))
                        return select(test0, B(2.) - r1, r1);
                }
                auto test2 = x < lim2;
                auto test3 = test2 && !test1;
                B ex = exp(-xx);
                if (any(test3))
                {
                    B z = ex * erf_kernel<B>::erfc2(x);
                    r1 = select(test1, r1, z);
                    if (all(test1 || test3))
                        return select(test0, B(2.) - r1, r1);
                }
                B z = ex * erf_kernel<B>::erfc3(x);
                r1 = select(test2, r1, z);
#ifndef XSIMD_NO_INFINITIES
                r1 = select(x == infinity<B>(), B(0.), r1);
#endif
                return select(test0, B(2.) - r1, r1);
            }
        };
    }

    template <class B>
    inline batch_type_t<B> erfc(const simd_base<B>& x)
    {
        return detail::erfc_impl<batch_type_t<B>>::compute(x());
    }
}

#endif
