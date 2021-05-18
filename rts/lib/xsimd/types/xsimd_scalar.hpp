/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SCALAR_HPP
#define XSIMD_SCALAR_HPP

#include <cmath>
#include <limits>

#include "xsimd_common_math.hpp"

namespace xsimd
{

    /*********************************************
     * scalar fallback for xsimd math operations *
     *********************************************/

    using std::abs;
    using std::acos;
    using std::acosh;
    using std::asin;
    using std::asinh;
    using std::atan2;
    using std::atan;
    using std::atanh;
    using std::cbrt;
    using std::ceil;
    using std::copysign;
    using std::cos;
    using std::cosh;
    using std::erf;
    using std::erfc;
    using std::exp2;
    using std::exp;
    using std::expm1;
    using std::fabs;
    using std::fdim;
    using std::fmax;
    using std::fmin;
    using std::floor;
    using std::fmod;
    using std::hypot;
    using std::lgamma;
    using std::ldexp;
    using std::log10;
    using std::log1p;
    using std::log2;
    using std::log;
    using std::modf;
    using std::nearbyint;
    using std::nextafter;
    using std::proj;
    using std::remainder;
    using std::rint;
    using std::rint;
    using std::round;
    using std::round;
    using std::sin;
    using std::sinh;
    using std::sqrt;
    using std::tan;
    using std::tanh;
    using std::tgamma;
    using std::trunc;

#ifndef _WIN32
    using std::isfinite;
    using std::isinf;
    using std::isnan;
#else
    // Windows defines catch all templates
    template <class T>
    typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    isfinite(T var)
    {
        return std::isfinite(var);
    }

    template <class T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    isfinite(T var)
    {
        return isfinite(double(var));
    }

    template <class T>
    typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    isinf(T var)
    {
        return std::isinf(var);
    }

    template <class T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    isinf(T var)
    {
        return isinf(double(var));
    }

    template <class T>
    typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    isnan(T var)
    {
        return std::isnan(var);
    }

    template <class T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    isnan(T var)
    {
        return isnan(double(var));
    }
#endif

#ifdef XSIMD_ENABLE_NUMPY_COMPLEX
    template <class T>
    bool isnan(std::complex<T> var)
    {
        return std::isnan(std::real(var)) || std::isnan(std::imag(var));
    }

    template <class T>
    bool isinf(std::complex<T> var)
    {
        return std::isinf(std::real(var)) || std::isinf(std::imag(var));
    }
#endif

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    using xtl::abs;
    using xtl::norm;
    using xtl::proj;
    using xtl::exp;
    using xtl::log;
    using xtl::log10;
    using xtl::pow;
    using xtl::sqrt;
    using xtl::sin;
    using xtl::cos;
    using xtl::tan;
    using xtl::asin;
    using xtl::acos;
    using xtl::atan;
    using xtl::sinh;
    using xtl::cosh;
    using xtl::tanh;
    using xtl::asinh;
    using xtl::acosh;
    using xtl::atanh;
#endif

    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool is_flint(const T& x)
    {
        return std::isnan(x - x) ? std::numeric_limits<T>::quiet_NaN() : x - std::trunc(x);
    }

    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool is_odd(const T& x)
    {
        return is_even(x - 1.);
    }

    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline bool is_even(const T& x)
    {
        return is_flint(x * T(0.5));
    }

    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T exp10(const T& x)
    {
        // FIXME: faster alternatives exist
        return std::pow(T(10), x);
    }

    namespace detail
    {
        template <class C>
        inline C expm1_complex_scalar_impl(const C& val)
        {
            using T = typename C::value_type;
            T isin = sin(val.imag());
            T rem1 = expm1(val.real());
            T re = rem1 + T(1.);
            T si = sin(val.imag() * T(0.5));
            return std::complex<T>(rem1 - T(2.) * re *si * si, re * isin);
        }
    }

    template <class T>
    inline std::complex<T> expm1(const std::complex<T>& val)
    {
        return detail::expm1_complex_scalar_impl(val);
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T, bool i3ec>
    inline xtl::xcomplex<T, T, i3ec> expm1(const xtl::xcomplex<T, T, i3ec>& val)
    {
        return detail::expm1_complex_scalar_impl(val);
    }
#endif

    namespace detail {
    }

    template <class T0, class T1>
    inline typename std::enable_if<std::is_integral<T1>::value, T0>::type
    pow(const T0& t0, const T1& t1)
    {
        return detail::ipow(t0, t1);
    }

    template <class T0, class T1>
    inline auto
    pow(const T0& t0, const T1& t1)
        -> typename std::enable_if<std::is_scalar<T0>::value && std::is_floating_point<T1>::value, decltype(std::pow(t0, t1))>::type
    {
        return std::pow(t0, t1);
    }

    template <class T0, class T1>
    inline typename std::enable_if<std::is_integral<T1>::value, std::complex<T0>>::type
    pow(const std::complex<T0>& t0, const T1& t1)
    {
        return detail::ipow(t0, t1);
    }

    template <class T0, class T1>
    inline typename std::enable_if<!std::is_integral<T1>::value, std::complex<T0>>::type
    pow(const std::complex<T0>& t0, const T1& t1)
    {
      return std::pow(t0, t1);
    }

    template <class T0, class T1>
    inline auto
    pow(const T0& t0, const std::complex<T1>& t1)
        -> typename std::enable_if<std::is_scalar<T0>::value, decltype(std::pow(t0, t1))>::type
    {
      return std::pow(t0, t1);
    }

    template <class T>
    inline auto bitofsign(T const& x) -> decltype(std::signbit(x))
    {
        return std::signbit(x);
    }

    template <class T>
    inline auto signbit(T const& v) -> decltype(bitofsign(v))
    {
        return bitofsign(v);
    }

    inline double sign(bool const &v)
    {
        return v;
    }

    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T sign(const T& v)
    {
        return v < T(0) ? T(-1.) : v == T(0) ? T(0.) : T(1.);
    }

    namespace detail
    {
        template <class C>
        inline C sign_complex_scalar_impl(const C& v)
        {
            using value_type = typename C::value_type;
            if (v.real())
            {
                return C(sign(v.real()), value_type(0));
            }
            else
            {
                return C(sign(v.imag()), value_type(0));
            }
        }
    }

    template <class T>
    inline std::complex<T> sign(const std::complex<T>& v)
    {
        return detail::sign_complex_scalar_impl(v);
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T, bool i3ec>
    inline xtl::xcomplex<T, T, i3ec> sign(const xtl::xcomplex<T, T, i3ec>& v)
    {
        return detail::sign_complex_scalar_impl(v);
    }
#endif

    template <class T>
    std::complex<T> log2(const std::complex<T>& val)
    {
        return log(val) / log(T(2));
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T, bool i3ec>
    inline xtl::xcomplex<T, T, i3ec> log2(const xtl::xcomplex<T, T, i3ec>& val)
    {
        return log(val) / log(T(2));
    }
#endif

    namespace detail
    {
        template <class C>
        inline C log1p_complex_scalar_impl(const C& val)
        {
            using T = typename C::value_type;
            C u = C(1.) + val;
            return u == C(1.) ? val : (u.real() <= T(0.) ? log(u) : log(u) * val / (u - C(1.)));
        }
    }

    template <class T>
    inline std::complex<T> log1p(const std::complex<T>& val)
    {
        return detail::log1p_complex_scalar_impl(val);
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T, bool i3ec>
    inline xtl::xcomplex<T, T, i3ec> log1p(const xtl::xcomplex<T, T, i3ec>& val)
    {
        return detail::log1p_complex_scalar_impl(val);
    }
#endif

    template <class T0, class T1>
    inline auto min(T0 const &self, T1 const &other) ->
        typename std::enable_if<std::is_scalar<T0>::value && std::is_scalar<T1>::value,
                                typename std::decay<decltype(self > other ? other : self)>::type>::type
    {
        return self > other ? other : self;
    }

    // numpy defines minimum operator on complex using lexical comparison
    template <class T0, class T1>
    inline std::complex<typename std::common_type<T0, T1>::type>
    min(std::complex<T0> const &self, std::complex<T1> const &other)
    {
        return (self.real() < other.real()) ? (self) : (self.real() == other.real() ? (self.imag() < other.imag() ? self : other) : other);
    }

    template <class T0, class T1>
    inline auto max(T0 const &self, T1 const &other) ->
        typename std::enable_if<std::is_scalar<T0>::value && std::is_scalar<T1>::value,
                                typename std::decay<decltype(self > other ? other : self)>::type>::type
    {
        return self < other ? other : self;
    }

    // numpy defines maximum operator on complex using lexical comparison
    template <class T0, class T1>
    inline std::complex<typename std::common_type<T0, T1>::type>
    max(std::complex<T0> const &self, std::complex<T1> const &other)
    {
        return (self.real() > other.real()) ? (self) : (self.real() == other.real() ? (self.imag() > other.imag() ? self : other) : other);
    }

    template <class T>
    inline typename std::enable_if<std::is_scalar<T>::value, T>::type fma(const T& a, const T& b, const T& c)
    {
        return std::fma(a, b, c);
    }

    namespace detail
    {
        template <class C>
        inline C fma_complex_scalar_impl(const C& a, const C& b, const C& c)
        {
            return {fms(a.real(), b.real(), fms(a.imag(), b.imag(), c.real())),
                    fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))};
        }
    }

    template <class T>
    inline std::complex<T> fma(const std::complex<T>& a, const std::complex<T>& b, const std::complex<T>& c)
    {
        return detail::fma_complex_scalar_impl(a, b, c);
    }


#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T, bool i3ec>
    inline xtl::xcomplex<T, T, i3ec> fma(const xtl::xcomplex<T, T, i3ec>& a, const xtl::xcomplex<T, T, i3ec>& b, const xtl::xcomplex<T, T, i3ec>& c)
    {
        return detail::fma_complex_scalar_impl(a, b, c);
    }
#endif

    inline void sincos(float val, float&s, float& c)
    {
#if defined(__APPLE__)
        __sincosf(val, &s, &c);
#elif defined(_GNU_SOURCE)
        ::sincosf(val, &s, &c);
#else
        s = std::sin(val);
        c = std::cos(val);
#endif
    }

    inline void sincos(double val, double&s, double& c)
    {
#if defined(__APPLE__)
        __sincos(val, &s, &c);
#elif defined(_GNU_SOURCE)
        ::sincos(val, &s, &c);
#else
        s = std::sin(val);
        c = std::cos(val);
#endif
    }

    template <class T>
    inline void sincos(const std::complex<T>& val, std::complex<T>& s, std::complex<T>& c)
    {
        s = std::sin(val);
        c = std::cos(val);
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T>
    inline void sincos(const xtl::xcomplex<T>& val, xtl::xcomplex<T>& s, xtl::xcomplex<T>& c)
    {
        s = sin(val);
        c = cos(val);
    }
#endif

    template <class T>
    inline T frexp(T const& val, int& exp)
    {
        return std::frexp(val, &exp);
    }

    template <class T>
    inline decltype(abs(std::declval<T>())) norm(const T& val)
    {
        auto tmp = abs(val);
        return tmp * tmp;
    }

    template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    T sadd(const T& lhs, const T& rhs)
    {
        if (std::numeric_limits<T>::is_signed)
        {
            if ((lhs > 0) && (rhs > std::numeric_limits<T>::max() - lhs))
            {
                return std::numeric_limits<T>::max();
            }
            else if ((lhs < 0) && (rhs < std::numeric_limits<T>::lowest() - lhs))
            {
                return std::numeric_limits<T>::lowest();
            }
            else {
                return lhs + rhs;
            }
        }
        else
        {
            if (rhs > std::numeric_limits<T>::max() - lhs)
            {
                return std::numeric_limits<T>::max();
            }
            else
            {
                return lhs + rhs;
            }

        }
    }

    template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    T ssub(const T& lhs, const T& rhs)
    {
        if (std::numeric_limits<T>::is_signed)
        {
            return sadd(lhs, (T)-rhs);
        }
        else
        {
            if (lhs < rhs)
            {
                return std::numeric_limits<T>::lowest();
            }
            else
            {
                return lhs - rhs;
            }

        }
    }

}

#endif
