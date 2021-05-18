/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_MATH_COMPLEX_HPP
#define XSIMD_MATH_COMPLEX_HPP

#include "../types/xsimd_complex_base.hpp"
#include "xsimd_basic_math.hpp"
#include "xsimd_exponential.hpp"
#include "xsimd_hyperbolic.hpp"
#include "xsimd_logarithm.hpp"
#include "xsimd_power.hpp"
#include "xsimd_trigonometric.hpp"

namespace xsimd
{
    template <class X>
    real_batch_type_t<X> real(const simd_base<X>& z);

    template <class X>
    real_batch_type_t<X> imag(const simd_base<X>& z);

    template <class X>
    real_batch_type_t<X> arg(const simd_base<X>& z);

    template <class X>
    batch_type_t<X> conj(const simd_base<X>& z);

    template <class X>
    real_batch_type_t<X> norm(const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> proj(const simd_base<X>& rhs);

    namespace detail
    {
        /********
         * sign *
         ********/

        template <class T, std::size_t N>
        inline batch<T, N> sign_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            auto rz = z.real();
            auto iz = z.imag();
            return select(rz != r_type(0.),
                          b_type(sign(rz)),
                          b_type(sign(iz)));
        }

        template <class T, std::size_t N>
        struct sign_impl<batch<std::complex<T>, N>, false>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return sign_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct sign_impl<batch<xtl::xcomplex<T, T, i3ec>, N>, false>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return sign_complex_impl(z);
            }
        };
#endif

        /*******
         * exp *
         *******/

        template <class T, std::size_t N>
        inline batch<T, N> exp_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            r_type icos, isin;
            sincos(z.imag(), isin, icos);
            return exp(z.real()) * batch<T, N>(icos, isin);
        }

        template <class T, std::size_t N>
        struct exp_kernel<batch<std::complex<T>, N>, exp_tag, std::complex<T>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return exp_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct exp_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>, exp_tag, xtl::xcomplex<T, T, i3ec>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return exp_complex_impl(z);
            }
        };
#endif

        /*********
         * expm1 *
         *********/

        template <class T, std::size_t N>
        inline batch<T, N> expm1_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            r_type isin = sin(z.imag());
            r_type rem1 = expm1(z.real());
            r_type re = rem1 + r_type(1.);
            r_type si = sin(z.imag() * r_type(0.5));
            return batch<T, N>(rem1 - r_type(2.) * re * si * si, re * isin);
        }

        template <class T, std::size_t N>
        struct expm1_kernel<batch<std::complex<T>, N>, std::complex<T>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return expm1_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct expm1_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>, xtl::xcomplex<T, T, i3ec>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return expm1_complex_impl(z);
            }
        };
#endif

        /*******
         * log *
         *******/

        template <class T, std::size_t N>
        inline batch<T, N> log_complex_impl(const batch<T, N>& z)
        {
            return batch<T, N>(log(abs(z)), atan2(z.imag(), z.real()));
        }

        template <class T, std::size_t N>
        struct log_kernel<batch<std::complex<T>, N>, std::complex<T>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return log_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct log_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>, xtl::xcomplex<T, T, i3ec>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return log_complex_impl(z);
            }
        };
#endif

        /*********************
         * logN_complex_impl *
         *********************/

        template <class T, std::size_t N>
        inline batch<T, N> logN_complex_impl(const batch<T, N>& z, typename batch<T, N>::real_value_type base)
        {
            using b_type = batch<T, N>;
            using rv_type = typename b_type::real_value_type;
            return log(z) / b_type(rv_type(base));
        }

        /********
         * log2 *
         ********/

        template <class T, std::size_t N>
        struct log2_kernel<batch<std::complex<T>, N>, std::complex<T>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return logN_complex_impl(z, std::log(2));
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct log2_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>, xtl::xcomplex<T, T, i3ec>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return logN_complex_impl(z, std::log(2));
            }
        };
#endif

        /*********
         * log10 *
         *********/

        template <class T, std::size_t N>
        struct log10_kernel<batch<std::complex<T>, N>, std::complex<T>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return logN_complex_impl(z, std::log(10));
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct log10_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>, xtl::xcomplex<T, T, i3ec>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return logN_complex_impl(z, std::log(10));
            }
        };
#endif

        /*********
         * log1p *
         *********/

        template <class T, std::size_t N>
        inline batch<T, N> log1p_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            b_type u = b_type(1.) + z;
            b_type logu = log(u);
            return select(u == b_type(1.),
                          z,
                          select(u.real() <= r_type(0.),
                                 logu,
                                 logu * z / (u - b_type(1.))));
        }

        template <class T, std::size_t N>
        struct log1p_kernel<batch<std::complex<T>, N>, std::complex<T>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return log1p_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct log1p_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>, xtl::xcomplex<T, T, i3ec>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return log1p_complex_impl(z);
            }
        };
#endif

        /*******
         * pow *
         *******/

        template <class T, std::size_t N>
        inline batch<T, N> pow_complex_impl(const batch<T, N>& a, const batch<T, N>& z)
        {
            using cplx_batch = batch<T, N>;
            using real_batch = typename cplx_batch::real_batch;
            real_batch absa = abs(a);
            real_batch arga = arg(a);
            real_batch x = z.real();
            real_batch y = z.imag();
            real_batch r = pow(absa, x);
            real_batch theta = x * arga;
            real_batch ze = zero<real_batch>();
            auto cond = (y == ze);
            r = select(cond, r, r * exp(-y * arga));
            theta = select(cond, theta, theta + y * log(absa));
            return select(absa == ze, cplx_batch(ze), cplx_batch(r * cos(theta), r * sin(theta)));
        }

        template <class T, std::size_t N>
        struct pow_kernel<batch<std::complex<T>, N>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& a, const batch_type& z)
            {
                return pow_complex_impl(a, z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct pow_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& a, const batch_type& z)
            {
                return pow_complex_impl(a, z);
            }
        };
#endif

        /*********
         * trigo *
         *********/

        template <class T, std::size_t N>
        inline void sincos_complex_impl(const batch<T, N>& z, batch<T, N>& si, batch<T, N>& co)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            r_type rcos = cos(z.real());
            r_type rsin = sin(z.real());
            r_type icosh = cosh(z.imag());
            r_type isinh = sinh(z.imag());
            si = b_type(rsin * icosh, rcos * isinh);
            co = b_type(rcos * icosh, -rsin * isinh);
        }

        template <class T, std::size_t N>
        inline batch<T, N> sin_complex_impl(const batch<T, N>& z)
        {
            return batch<T, N>(sin(z.real()) * cosh(z.imag()), cos(z.real()) * sinh(z.imag()));
        }

        template <class T, std::size_t N>
        inline batch<T, N> cos_complex_impl(const batch<T, N>& z)
        {
            return batch<T, N>(cos(z.real()) * cosh(z.imag()), -sin(z.real()) * sinh(z.imag()));
        }

        template <class T, std::size_t N>
        inline batch<T, N> tan_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            r_type d = cos(2 * z.real()) + cosh(2 * z.imag());
            b_type winf(infinity<r_type>(), infinity<r_type>());
            r_type wreal = sin(2 * z.real()) / d;
            r_type wimag = sinh(2 * z.imag());
            b_type wres = select(xsimd::isinf(wimag), b_type(wreal, r_type(1.)), b_type(wreal, wimag / d));
            return select(d == r_type(0.), winf, wres);
        }

        template <class T, std::size_t N>
        struct trigo_kernel<batch<std::complex<T>, N>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type sin(const batch_type& z)
            {
                return sin_complex_impl(z);
            }

            static inline batch_type cos(const batch_type& z)
            {
                return cos_complex_impl(z);
            }

            static inline batch_type tan(const batch_type& z)
            {
                return tan_complex_impl(z);
            }

            static inline void sincos(const batch_type& z, batch_type& si, batch_type& co)
            {
                return sincos_complex_impl(z, si, co);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct trigo_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type sin(const batch_type& z)
            {
                return sin_complex_impl(z);
            }

            static inline batch_type cos(const batch_type& z)
            {
                return cos_complex_impl(z);
            }

            static inline batch_type tan(const batch_type& z)
            {
                return tan_complex_impl(z);
            }

            static inline void sincos(const batch_type& z, batch_type& si, batch_type& co)
            {
                return sincos_complex_impl(z, si, co);
            }
        };
#endif

        /************
         * invtrigo *
         ************/

        template <class T, std::size_t N>
        batch<T, N> asin_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;

            r_type x = z.real();
            r_type y = z.imag();

            b_type ct(-y, x);
            b_type zz(r_type(1.) - (x - y) * (x + y), -2 * x * y);
            zz = log(ct + sqrt(zz));
            b_type resg(zz.imag(), -zz.real());

            return select(y == r_type(0.),
                          select(fabs(x) > r_type(1.),
                                 b_type(pio2<r_type>(), r_type(0.)),
                                 b_type(asin(x), r_type(0.))),
                          resg);
        }

        template <class T, std::size_t N>
        batch<T, N> acos_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            b_type tmp = asin_complex_impl(z);
            return b_type(pio2<r_type>() - tmp.real(), -tmp.imag());
        }

        template <class T, std::size_t N>
        batch<T, N> atan_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            using r_type = typename b_type::real_batch;
            r_type x = z.real();
            r_type y = z.imag();
            r_type x2 = x * x;
            r_type one = r_type(1.);
            r_type a = one - x2 - (y * y);
            r_type w = 0.5 * atan2(2. * x, a);
            r_type num = y + one;
            num = x2 + num * num;
            r_type den = y - one;
            den = x2 + den * den;
            b_type res = select((x == r_type(0.)) && (y == r_type(1.)),
                                b_type(r_type(0.), infinity<r_type>()),
                                b_type(w, 0.25 * log(num / den)));
            return res;
        }

        template <class T, std::size_t N>
        struct invtrigo_kernel<batch<std::complex<T>, N>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type asin(const batch_type& z)
            {
                return asin_complex_impl(z);
            }

            static inline batch_type acos(const batch_type& z)
            {
                return acos_complex_impl(z);
            }

            static inline batch_type atan(const batch_type& z)
            {
                return atan_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct invtrigo_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type asin(const batch_type& z)
            {
                return asin_complex_impl(z);
            }

            static inline batch_type acos(const batch_type& z)
            {
                return acos_complex_impl(z);
            }

            static inline batch_type atan(const batch_type& z)
            {
                return atan_complex_impl(z);
            }
        };
#endif

        /********
         * sinh *
         ********/

        template <class T, std::size_t N>
        inline batch<T, N> sinh_complex_impl(const batch<T, N>& z)
        {
            auto x = z.real();
            auto y = z.imag();
            return batch<T, N>(sinh(x) * cos(y), cosh(x) * sin(y));
        }

        template <class T, std::size_t N>
        struct sinh_kernel<batch<std::complex<T>, N>>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return sinh_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct sinh_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return sinh_complex_impl(z);
            }
        };
#endif

        /********
         * cosh *
         ********/

        template <class T, std::size_t N>
        inline batch<T, N> cosh_complex_impl(const batch<T, N>& z)
        {
            auto x = z.real();
            auto y = z.imag();
            return batch<T, N>(cosh(x) * cos(y), sinh(x) * sin(y));
        }

        template <class T, std::size_t N>
        struct cosh_kernel<batch<std::complex<T>, N >>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return cosh_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct cosh_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return cosh_complex_impl(z);
            }
        };
#endif

        /********
         * tanh *
         ********/

        template <class T, std::size_t N>
        inline batch<T, N> tanh_complex_impl(const batch<T, N>& z)
        {
            using rvt = typename batch<T, N>::real_value_type;
            using real_batch = typename batch<T, N>::real_batch;
            auto x = z.real();
            auto y = z.imag();
            real_batch two = real_batch(rvt(2));
            auto d = cosh(two * x) + cos(two * y);
            return batch<T, N>(sinh(two * x) / d, sin(two * y) / d);
        }

        template <class T, std::size_t N>
        struct tanh_kernel<batch<std::complex<T>, N >>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return tanh_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct tanh_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return tanh_complex_impl(z);
            }
        };
#endif

        /*********
         * asinh *
         *********/

        template <class T, std::size_t N>
        inline batch<T, N> asinh_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            b_type w = asin(b_type(-z.imag(), z.real()));
            w = b_type(w.imag(), -w.real());
            return w;
        }

        template <class T, std::size_t N>
        struct asinh_kernel<batch<std::complex<T>, N >>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return asinh_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct asinh_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return asinh_complex_impl(z);
            }
        };
#endif

        /*********
         * acosh *
         *********/

        template <class T, std::size_t N>
        inline batch<T, N> acosh_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            b_type w = acos(z);
            w = b_type(-w.imag(), w.real());
            return w;
        }

        template <class T, std::size_t N>
        struct acosh_kernel<batch<std::complex<T>, N >>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return acosh_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct acosh_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return acosh_complex_impl(z);
            }
        };
#endif

        /*********
         * atanh *
         *********/

        template <class T, std::size_t N>
        inline batch<T, N> atanh_complex_impl(const batch<T, N>& z)
        {
            using b_type = batch<T, N>;
            b_type w = atan(b_type(-z.imag(), z.real()));
            w = b_type(w.imag(), -w.real());
            return w;
        }

        template <class T, std::size_t N>
        struct atanh_kernel<batch<std::complex<T>, N >>
        {
            using batch_type = batch<std::complex<T>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return atanh_complex_impl(z);
            }
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct atanh_kernel<batch<xtl::xcomplex<T, T, i3ec>, N>>
        {
            using batch_type = batch<xtl::xcomplex<T, T, i3ec>, N>;

            static inline batch_type compute(const batch_type& z)
            {
                return atanh_complex_impl(z);
            }
        };
#endif
    }

    namespace detail
    {
        template <class T>
        T csqrt_scale_factor() noexcept;

        template <class T>
        T csqrt_scale() noexcept;

        template <>
        inline float csqrt_scale_factor<float>() noexcept
        {
            return 6.7108864e7f;
        }

        template <>
        inline float csqrt_scale<float>() noexcept
        {
            return 1.220703125e-4f;
        }

        template <>
        inline double csqrt_scale_factor<double>() noexcept
        {
            return 1.8014398509481984e16;
        }

        template <>
        inline double csqrt_scale<double>() noexcept
        {
            return 7.450580596923828125e-9;
        }
    }

    namespace detail
    {
        template <class T, std::size_t N>
        struct complex_batch_kernel
        {
            using batch_type = batch<T, N>;
            using batch_bool_type = typename simd_batch_traits<batch_type>::batch_bool_type;
            using real_batch = typename batch_type::real_batch;

            static real_batch abs(const batch_type& z)
            {
                return hypot(z.real(), z.imag());
            }

            static batch_type fma(const batch_type& a, const batch_type& b, const batch_type& c)
            {
                real_batch res_r = ::xsimd::fms(a.real(), b.real(), ::xsimd::fms(a.imag(), b.imag(), c.real()));
                real_batch res_i = ::xsimd::fma(a.real(), b.imag(), ::xsimd::fma(a.imag(), b.real(), c.imag()));
                return {res_r, res_i};
            }

            static batch_type fms(const batch_type& a, const batch_type& b, const batch_type& c)
            {
                real_batch res_r = ::xsimd::fms(a.real(), b.real(), ::xsimd::fma(a.imag(), b.imag(), c.real()));
                real_batch res_i = ::xsimd::fma(a.real(), b.imag(), ::xsimd::fms(a.imag(), b.real(), c.imag()));
                return {res_r, res_i};
            }

            static batch_type fnma(const batch_type& a, const batch_type& b, const batch_type& c)
            {
                real_batch res_r = - ::xsimd::fms(a.real(), b.real(), ::xsimd::fma(a.imag(), b.imag(), c.real()));
                real_batch res_i = - ::xsimd::fma(a.real(), b.imag(), ::xsimd::fms(a.imag(), b.real(), c.imag()));
                return {res_r, res_i};
            }

            static batch_type fnms(const batch_type& a, const batch_type& b, const batch_type& c)
            {
                real_batch res_r = - ::xsimd::fms(a.real(), b.real(), ::xsimd::fms(a.imag(), b.imag(), c.real()));
                real_batch res_i = - ::xsimd::fma(a.real(), b.imag(), ::xsimd::fma(a.imag(), b.real(), c.imag()));
                return {res_r, res_i};
            }

            static batch_type sqrt(const batch_type& z)
            {
                using rvt = typename real_batch::value_type;
                real_batch x = z.real();
                real_batch y = z.imag();
                real_batch sqrt_x = xsimd::sqrt(fabs(x));
                real_batch sqrt_hy = xsimd::sqrt(0.5 * fabs(y));
                auto cond = (fabs(x) > real_batch(4.) || fabs(y) > real_batch(4.));
                x = select(cond, x * 0.25, x * detail::csqrt_scale_factor<rvt>());
                y = select(cond, y * 0.25, y * detail::csqrt_scale_factor<rvt>());
                real_batch scale = select(cond, real_batch(2.), real_batch(detail::csqrt_scale<rvt>()));
                real_batch r = abs(batch_type(x, y));

                auto condxp = x > real_batch(0.);
                real_batch t0 = select(condxp, xsimd::sqrt(0.5 * (r + x)), xsimd::sqrt(0.5 * (r - x)));
                real_batch r0 = scale * fabs((0.5 * y) / t0);
                t0 *= scale;
                real_batch t = select(condxp, t0, r0);
                r = select(condxp, r0, t0);
                batch_type resg = select(y < real_batch(0.), batch_type(t, -r), batch_type(t, r));
                real_batch ze(0.);

                return select(y == ze,
                              select(x == ze,
                                     batch_type(ze, ze),
                                     select(x < ze, batch_type(ze, sqrt_x), batch_type(sqrt_x, ze))),
                              select(x == ze,
                                     select(y > ze, batch_type(sqrt_hy, sqrt_hy), batch_type(sqrt_hy, -sqrt_hy)),
                                     resg));
            }

            static batch_bool_type isnan(const batch_type& z)
            {
                return batch_bool_type(xsimd::isnan(z.real()) || xsimd::isnan(z.imag()));
            }
        };

        template <class T, std::size_t N>
        struct batch_kernel<std::complex<T>, N>
             : public complex_batch_kernel<std::complex<T>, N>
        {
        };

        #ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, bool i3ec, std::size_t N>
        struct batch_kernel<xtl::xcomplex<T, T, i3ec>, N>
             : public complex_batch_kernel<xtl::xcomplex<T, T, i3ec>, N>
        {
        };
        #endif
    }

    namespace detail
    {
        template <class B, bool is_complex>
        struct real_imag_kernel
        {
            using return_type = typename B::real_batch;

            static return_type real(const B& z)
            {
                return z.real();
            }

            static return_type imag(const B& z)
            {
                return z.imag();
            }
        };

        template <class B>
        struct real_imag_kernel<B, false>
        {
            using return_type = B;

            static return_type real(const B& z)
            {
                return z;
            }

            static return_type imag(const B&)
            {
                return B(typename B::value_type(0));
            }
        };
    }

    template <class X>
    inline real_batch_type_t<X> real(const simd_base<X>& z)
    {
        using batch_type = batch_type_t<X>;
        constexpr bool is_cplx = detail::is_complex<typename batch_type::value_type>::value;
        return detail::real_imag_kernel<batch_type, is_cplx>::real(z());
    }

    template <class X>
    inline real_batch_type_t<X> imag(const simd_base<X>& z)
    {
        using batch_type = batch_type_t<X>;
        constexpr bool is_cplx = detail::is_complex<typename batch_type::value_type>::value;
        return detail::real_imag_kernel<batch_type, is_cplx>::imag(z());
    }

    template <class X>
    inline real_batch_type_t<X> arg(const simd_base<X>& z)
    {
        return atan2(imag(z), real(z));
    }

    template <class X>
    inline batch_type_t<X> conj(const simd_base<X>& z)
    {
        return X(z().real(), -z().imag());
    }

    template <class X>
    inline real_batch_type_t<X> norm(const simd_base<X>& rhs)
    {
        return real(rhs) * real(rhs) + imag(rhs) * imag(rhs);
    }

    template <class X>
    inline batch_type_t<X> proj(const simd_base<X>& rhs)
    {
        using batch_type = batch_type_t<X>;
        using real_batch = typename simd_batch_traits<X>::real_batch;
        auto cond = xsimd::isinf(rhs().real()) || xsimd::isinf(rhs().imag());
        return select(cond, batch_type(infinity<real_batch>(), copysign(real_batch(0.), rhs().imag())), rhs());
    }
}

#endif
