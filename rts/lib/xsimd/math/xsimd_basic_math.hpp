/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_BASIC_MATH_HPP
#define XSIMD_BASIC_MATH_HPP

#include "xsimd_numerical_constant.hpp"
#include "xsimd_rounding.hpp"
#include "../types/xsimd_traits.hpp"

namespace xsimd
{
    /********************
     * Basic operations *
     ********************/

    template <class B>
    batch_type_t<B> fmod(const simd_base<B>& x, const simd_base<B>& y);

    template <class B>
    batch_type_t<B> remainder(const simd_base<B>& x, const simd_base<B>& y);

    template <class B>
    batch_type_t<B> fdim(const simd_base<B>& x, const simd_base<B>& y);

    template <class B>
    batch_type_t<B> clip(const simd_base<B>& x, const simd_base<B>& lo, const simd_base<B>& hi);

    template <class X, class Y>
    batch_type_t<X> nextafter(const simd_base<X>& from, const simd_base<Y>& to);

    /****************************
     * Classification functions *
     ****************************/

    template <class B>
    typename simd_batch_traits<B>::batch_bool_type
    isfinite(const simd_base<B>& x);

    template <class B>
    typename simd_batch_traits<B>::batch_bool_type
    isinf(const simd_base<B>& x);

    template <class B>
    typename simd_batch_traits<B>::batch_bool_type
    is_flint(const simd_base<B>& x);

    template <class B>
    typename simd_batch_traits<B>::batch_bool_type
    is_odd(const simd_base<B>& x);

    template <class B>
    typename simd_batch_traits<B>::batch_bool_type
    is_even(const simd_base<B>& x);

    /***********************************
     * Basic operations implementation *
     ***********************************/

    namespace detail
    {

        template <class T>
        struct get_batch_value_type
        {
            using type = T;
        };

        template <class T, std::size_t N>
        struct get_batch_value_type<batch<T, N>>
        {
            using type = T;
        };

        template <class B>
        using get_batch_value_type_t = typename get_batch_value_type<B>::type;

        template <class B, bool is_int = std::is_integral<get_batch_value_type_t<B>>::value>
        struct remainder_kernel
        {
            using batch_type = B;
            using size_type = std::size_t;
            static constexpr size_type double_size = batch_type::size;

            static inline batch_type fmod(const batch_type& x, const batch_type& y)
            {
                return fnma(trunc(x / y), y, x);
            }

            static inline batch_type remainder(const batch_type& x, const batch_type& y)
            {
                return fnma(nearbyint(x / y), y, x);
            }

            template <class IB>
            static inline batch_type to_double(const IB& b, size_type offset)
            {
                batch_type res;
                for (size_type i = 0; i < double_size; ++i)
                {
                    res[i] = b[i + offset];
                }
                return res;
            }

            template <class IB>
            static inline void to_int(const batch_type& src, IB& dst, size_type offset)
            {
                for (size_type i = 0; i < double_size; ++i)
                {
                    dst[i + offset] = src[i];
                }
            }
        };

        template <>
        struct remainder_kernel<double, false>
        {
            using size_type = std::size_t;
            static inline double fmod(double x, double y)
            {
                return std::fmod(x, y);
            }

            static inline double remainder(double x, double y)
            {
                return std::remainder(x, y);
            }

            template <class IB>
            static inline double to_double(const IB& b, size_type offset)
            {
                double res = b[offset];
                return res;
            }

            template <class IB>
            static inline void to_int(double src, IB& dst, size_type offset)
            {
                dst[offset] = src;
            }
        };

        template <class B>
        struct remainder_kernel<B, true>
        {
            using batch_type = B;
            using double_batch = typename simd_traits<double>::type;
            using double_kernel = remainder_kernel<double_batch>;
            using size_type = std::size_t;
            static constexpr size_type int_size = B::size;
            static constexpr size_type double_size = simd_traits<double>::size;

            static inline batch_type fmod(const batch_type& x, const batch_type& y)
            {
                batch_type res;
                for (size_type i = 0; i < int_size; i += double_size)
                {
                    double_batch tmp = double_kernel::fmod(double_kernel::to_double(x, i),
                                                           double_kernel::to_double(y, i));
                    double_kernel::to_int(tmp, res, i);
                }
                return res;
            }

            static inline batch_type remainder(const batch_type& x, const batch_type& y)
            {
                batch_type res;
                for (size_type i = 0; i < int_size; i += double_size)
                {
                    double_batch tmp = double_kernel::remainder(double_kernel::to_double(x, i),
                                                                double_kernel::to_double(y, i));
                    double_kernel::to_int(tmp, res, i);
                }
                return res;
            }
        };
    }

    /**
     * @brief Computes the floating-point remainder of the division operation \c x/y.
     *
     * The floating-point remainder of the division operation \c x/y calculated by this
     * function is exactly the value <tt>x - n*y</tt>, where \c n is \c x/y with its fractional
     * part truncated. The returned value has the same sign as \c x and is less than \c y in magnitude.
     * @param x batch of floating point values.
     * @param y batch of floating point values.
     * @return the floating-point remainder of the division.
     */
    template <class B>
    inline batch_type_t<B> fmod(const simd_base<B>& x, const simd_base<B>& y)
    {
        return detail::remainder_kernel<B>::fmod(x(), y());
    }

    /**
     * @brief Computes the IEEE remainder of the floating point division operation \c x/y.
     *
     * The IEEE floating-point remainder of the division operation \c x/y calculated by this
     * function is exactly the value <tt>x - n*y</tt>, where the value n is the integral value
     * nearest the exact value \c x/y. When <tt>|n-x/y| = 0.5</tt>, the value n is chosen to be even.
     * In contrast to fmod, the returned value is not guaranteed to have the same sign as \c x.
     * If the returned value is 0, it will have the same sign as \c x.
     * @param x batch of floating point values.
     * @param y batch of floating point values.
     * @return the IEEE remainder remainder of the floating point division.
     */
    template <class B>
    inline batch_type_t<B> remainder(const simd_base<B>& x, const simd_base<B>& y)
    {
        return detail::remainder_kernel<B>::remainder(x(), y());
    }

    /**
     * Computes the positive difference between \c x and \c y, that is,
     * <tt>max(0, x-y)</tt>.
     * @param x batch of floating point values.
     * @param y batch of floating point values.
     * @return the positive difference.
     */
    template <class B>
    inline batch_type_t<B> fdim(const simd_base<B>& x, const simd_base<B>& y)
    {
        return fmax(batch_type_t<B>(0.), x - y);
    }

    /**
     * Clips the values of the batch \c x between those of the batches \c lo and \c hi.
     * @param x batch of floating point values.
     * @param lo batch of floating point values.
     * @param hi batch of floating point values.
     * @return the result of the clipping.
     */
    template <class B>
    inline batch_type_t<B> clip(const simd_base<B>& x, const simd_base<B>& lo, const simd_base<B>& hi)
    {
        return min(hi, max(x, lo));
    }

    // TODO move scalar version?
    template <class T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
    inline T clip(const T& x, const T& lo, const T& hi)
    {
        return std::min(hi, std::max(x, lo));
    }

    namespace detail
    {
        template <class T, std::size_t N, bool is_int = std::is_integral<T>::value>
        struct nextafter_kernel
        {
            using batch_type = batch<T, N>;

            static inline batch_type next(const batch_type& b) noexcept
            {
                return b;
            }

            static inline batch_type prev(const batch_type& b) noexcept
            {
                return b;
            }
        };

        template <class T, std::size_t N>
        struct bitwise_cast_batch;

        template <std::size_t N>
        struct bitwise_cast_batch<float, N>
        {
            using type = batch<int32_t, N>;
        };

        template <std::size_t N>
        struct bitwise_cast_batch<double, N>
        {
            using type = batch<int64_t, N>;
        };

        template <class T, std::size_t N>
        struct nextafter_kernel<T, N, false>
        {
            using batch_type = batch<T, N>;
            using int_batch = typename bitwise_cast_batch<T, N>::type;
            using int_type = typename int_batch::value_type;

            static inline batch_type next(const batch_type& b) noexcept
            {
                batch_type n = bitwise_cast<batch_type>(bitwise_cast<int_batch>(b) + int_type(1));
                return select(b == infinity<batch_type>(), b, n);
            }

            static inline batch_type prev(const batch_type& b) noexcept
            {
                batch_type p = bitwise_cast<batch_type>(bitwise_cast<int_batch>(b) - int_type(1));
                return select(b == minusinfinity<batch_type>(), b, p);
            }
        };
    }

    template <class X, class Y>
    inline batch_type_t<X> nextafter(const simd_base<X>& from, const simd_base<Y>& to)
    {
        using kernel = detail::nextafter_kernel<typename batch_type_t<X>::value_type, batch_type_t<X>::size>;
        return select(from == to,
                      from,
                      select(to > from,
                             kernel::next(from()),
                             kernel::prev(from())));
    }

    /*******************************************
     * Classification functions implementation *
     *******************************************/

    /**
     * Determines if the scalars in the given batch \c x are finite values,
     * i.e. they are different from infinite or NaN.
     * @param x batch of floating point values.
     * @return a batch of booleans.
     */
    template <class B>
    inline typename simd_batch_traits<B>::batch_bool_type
    isfinite(const simd_base<B>& x)
    {
        return (x - x) == batch_type_t<B>(0.);
    }

    namespace detail
    {
        template <class T, std::size_t N, bool is_int = std::is_integral<T>::value>
        struct isinf_kernel
        {
            using batch_type = batch<T, N>;
            using batch_bool_type = typename simd_batch_traits<batch_type>::batch_bool_type;

            static inline batch_bool_type run(const batch_type& x)
            {
                return  abs(x) == ::xsimd::infinity<batch_type_t<batch_type>>();
            }
        };

        template <class T, std::size_t N>
        struct isinf_kernel<T, N, true>
        {

            using batch_type = batch<T, N>;
            using batch_bool_type = typename simd_batch_traits<batch_type>::batch_bool_type;

            static inline batch_bool_type run(const batch_type&)
            {
                return batch_bool_type(false);
            }
        };
    }

    /**
     * Determines if the scalars in the given batch \c x are positive
     * or negative infinity.
     * @param x batch of floating point values.
     * @return a batch of booleans.
     */
    template <class B>
    inline typename simd_batch_traits<B>::batch_bool_type
    isinf(const simd_base<B>& x)
    {
        using kernel_type = detail::isinf_kernel<typename batch_type_t<B>::value_type, batch_type_t<B>::size>;
        return kernel_type::run(x());
        //return abs(x) == infinity<batch_type_t<B>>();
    }

    template <class B>
    inline typename simd_batch_traits<B>::batch_bool_type
    is_flint(const simd_base<B>& x)
    {
        using b_type = batch_type_t<B>;
        b_type frac = select(xsimd::isnan(x - x), nan<b_type>(), x - trunc(x));
        return frac == b_type(0.);
    }

    template <class B>
    inline typename simd_batch_traits<B>::batch_bool_type
    is_odd(const simd_base<B>& x)
    {
        return is_even(x - batch_type_t<B>(1.));
    }

    template <class B>
    inline typename simd_batch_traits<B>::batch_bool_type
    is_even(const simd_base<B>& x)
    {
        return is_flint(x * batch_type_t<B>(0.5));
    }
}

#endif
