/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ROUNDING_HPP
#define XSIMD_ROUNDING_HPP

#include <cmath>

#include "xsimd_fp_sign.hpp"
#include "xsimd_numerical_constant.hpp"

namespace xsimd
{
    /**
     * Computes the batch of smallest integer values not less than
     * scalars in \c x.
     * @param x batch of floating point values.
     * @return the batch of smallest integer values not less than \c x.
     */
    template <class B>
    batch_type_t<B> ceil(const simd_base<B>& x);

    /**
     * Computes the batch of largest integer values not greater than
     * scalars in \c x.
     * @param x batch of floating point values.
     * @return the batch of largest integer values not greater than \c x.
     */
    template <class B>
    batch_type_t<B> floor(const simd_base<B>& x);

    /**
     * Computes the batch of nearest integer values not greater in magnitude
     * than scalars in \c x.
     * @param x batch of floating point values.
     * @return the batch of nearest integer values not greater in magnitude than \c x.
     */
    template <class B>
    batch_type_t<B> trunc(const simd_base<B>& x);

    /**
     * Computes the batch of nearest integer values to scalars in \c x (in
     * floating point format), rounding halfway cases away from zero, regardless
     * of the current rounding mode.
     * @param x batch of flaoting point values.
     * @return the batch of nearest integer values. 
     */
    template <class B>
    batch_type_t<B> round(const simd_base<B>& x);

    // Contrary to their std counterpart, these functions
    // are assume that the rounding mode is FE_TONEAREST

    /**
     * Rounds the scalars in \c x to integer values (in floating point format), using
     * the current rounding mode.
     * @param x batch of flaoting point values.
     * @return the batch of nearest integer values.
     */
    template <class B>
    batch_type_t<B> nearbyint(const simd_base<B>& x);

    /**
     * Rounds the scalars in \c x to integer values (in floating point format), using
     * the current rounding mode.
     * @param x batch of flaoting point values.
     * @return the batch of rounded values.
     */
    template <class B>
    batch_type_t<B> rint(const simd_base<B>& x);

    namespace impl
    {
        template <class B>
        struct rounding_kernel;

        template <class B>
        struct rounding_kernel_int
        {
            static inline B ceil(const B& x)
            {
                return x;
            }

            static inline B floor(const B& x)
            {
                return x;
            }

            static inline B trunc(const B& x)
            {
                return x;
            }

            static inline B nearbyint(const B& x)
            {
                return x;
            }
        };

#define DEFINE_ROUNDING_KERNEL_INT(T, N)       \
        template <>                            \
        struct rounding_kernel<batch<T, N>>    \
            : rounding_kernel_int<batch<T, N>> \
        {                                      \
        }

        /**********************
         * SSE implementation *
         **********************/

    #if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION

        DEFINE_ROUNDING_KERNEL_INT(uint8_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(int8_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(uint16_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(int16_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(uint32_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(int32_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(uint64_t, 2);
        DEFINE_ROUNDING_KERNEL_INT(int64_t, 2);

        template <>
        struct rounding_kernel<batch<float, 4>>
        {
            using batch_type = batch<float, 4>;

            static inline batch_type ceil(const batch_type& x)
            {
                return _mm_ceil_ps(x);
            }

            static inline batch_type floor(const batch_type& x)
            {
                return _mm_floor_ps(x);
            }

            static inline batch_type trunc(const batch_type& x)
            {
                return _mm_round_ps(x, _MM_FROUND_TO_ZERO);
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                return _mm_round_ps(x, _MM_FROUND_TO_NEAREST_INT);
            }
        };

        template <>
        struct rounding_kernel<batch<double, 2>>
        {
            using batch_type = batch<double, 2>;

            static inline batch_type ceil(const batch_type& x)
            {
                return _mm_ceil_pd(x);
            }

            static inline batch_type floor(const batch_type& x)
            {
                return _mm_floor_pd(x);
            }

            static inline batch_type trunc(const batch_type& x)
            {
                return _mm_round_pd(x, _MM_FROUND_TO_ZERO);
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                return _mm_round_pd(x, _MM_FROUND_TO_NEAREST_INT);
            }
        };

    #elif (XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE2_VERSION) || (XSIMD_ARM_INSTR_SET == XSIMD_ARM7_NEON_VERSION)

        DEFINE_ROUNDING_KERNEL_INT(uint8_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(int8_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(uint16_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(int16_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(uint32_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(int32_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(uint64_t, 2);
        DEFINE_ROUNDING_KERNEL_INT(int64_t, 2);

        template <class B>
        struct rounding_kernel_base
        {
            static inline B ceil(const B& x)
            {
                B tx = trunc(x);
                return select(tx < x, tx + B(1), tx);
            }

            static inline B floor(const B& x)
            {
                B tx = trunc(x);
                return select(tx > x, tx - B(1), tx);
            }

            static inline B nearbyint(const B& x)
            {
                B s = bitofsign(x);
                B v = x ^ s;
                B t2n = twotonmb<B>();
                B d0 = v + t2n;
                return s ^ select(v < t2n, d0 - t2n, v);
            }
        };

        template <>
        struct rounding_kernel<batch<float, 4>> : rounding_kernel_base<batch<float, 4>>
        {
            using batch_type = batch<float, 4>;

            static inline batch_type trunc(const batch_type& x)
            {
                return select(abs(x) < maxflint<batch_type>(), to_float(to_int(x)), x);
            }
        };

    #if (XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE2_VERSION)

        template <>
        struct rounding_kernel<batch<double, 2>> : rounding_kernel_base<batch<double, 2>>
        {
            using batch_type = batch<double, 2>;

            static inline batch_type trunc(const batch_type& x)
            {
                return batch<double, 2>(std::trunc(x[0]), std::trunc(x[1]));
            }
        };

    #endif
    #endif

        /**********************
         * AVX implementation *
         **********************/

    #if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX_VERSION

        DEFINE_ROUNDING_KERNEL_INT(uint8_t, 32);
        DEFINE_ROUNDING_KERNEL_INT(int8_t, 32);
        DEFINE_ROUNDING_KERNEL_INT(uint16_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(int16_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(uint32_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(int32_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(uint64_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(int64_t, 4);

        template <>
        struct rounding_kernel<batch<float, 8>>
        {
            using batch_type = batch<float, 8>;

            static inline batch_type ceil(const batch_type& x)
            {
                return _mm256_round_ps(x, _MM_FROUND_CEIL);
            }

            static inline batch_type floor(const batch_type& x)
            {
                return _mm256_round_ps(x, _MM_FROUND_FLOOR);
            }

            static inline batch_type trunc(const batch_type& x)
            {
                return _mm256_round_ps(x, _MM_FROUND_TO_ZERO);
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                return _mm256_round_ps(x, _MM_FROUND_TO_NEAREST_INT);
            }
        };

        template <>
        struct rounding_kernel<batch<double, 4>>
        {
            using batch_type = batch<double, 4>;

            static inline batch_type ceil(const batch_type& x)
            {
                return _mm256_round_pd(x, _MM_FROUND_CEIL);
            }

            static inline batch_type floor(const batch_type& x)
            {
                return _mm256_round_pd(x, _MM_FROUND_FLOOR);
            }

            static inline batch_type trunc(const batch_type& x)
            {
                return _mm256_round_pd(x, _MM_FROUND_TO_ZERO);
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                return _mm256_round_pd(x, _MM_FROUND_TO_NEAREST_INT);
            }
        };

    #endif

    #if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX512_VERSION

        DEFINE_ROUNDING_KERNEL_INT(uint8_t, 64);
        DEFINE_ROUNDING_KERNEL_INT(int8_t, 64);
        DEFINE_ROUNDING_KERNEL_INT(uint16_t, 32);
        DEFINE_ROUNDING_KERNEL_INT(int16_t, 32);
        DEFINE_ROUNDING_KERNEL_INT(uint32_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(int32_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(uint64_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(int64_t, 8);

        template <>
        struct rounding_kernel<batch<float, 16>>
        {
            using batch_type = batch<float, 16>;

            static inline batch_type ceil(const batch_type& x)
            {
                auto res = _mm512_roundscale_ps(x, _MM_FROUND_TO_POS_INF);
                return res;
            }

            static inline batch_type floor(const batch_type& x)
            {
                auto res = _mm512_roundscale_ps(x, _MM_FROUND_TO_NEG_INF);
                return res;
            }

            static inline batch_type trunc(const batch_type& x)
            {
                auto res = _mm512_roundscale_round_ps(x, _MM_FROUND_TO_ZERO, _MM_FROUND_CUR_DIRECTION);
                return res;
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                auto res = _mm512_roundscale_round_ps(x, _MM_FROUND_TO_NEAREST_INT, _MM_FROUND_CUR_DIRECTION);
                return res;
            }
        };

        template <>
        struct rounding_kernel<batch<double, 8>>
        {
            using batch_type = batch<double, 8>;

            static inline batch_type ceil(const batch_type& x)
            {
                auto res = _mm512_roundscale_pd(x, _MM_FROUND_TO_POS_INF);
                return res;
            }

            static inline batch_type floor(const batch_type& x)
            {
                auto res = _mm512_roundscale_pd(x, _MM_FROUND_TO_NEG_INF);
                return res;
            }

            static inline batch_type trunc(const batch_type& x)
            {
                auto res = _mm512_roundscale_round_pd(x, _MM_FROUND_TO_ZERO, _MM_FROUND_CUR_DIRECTION);
                return res;
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                auto res = _mm512_roundscale_round_pd(x, _MM_FROUND_TO_NEAREST_INT, _MM_FROUND_CUR_DIRECTION);
                return res;
            }
        };

    #endif

    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_32_NEON_VERSION

        DEFINE_ROUNDING_KERNEL_INT(uint8_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(int8_t, 16);
        DEFINE_ROUNDING_KERNEL_INT(uint16_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(int16_t, 8);
        DEFINE_ROUNDING_KERNEL_INT(uint32_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(int32_t, 4);
        DEFINE_ROUNDING_KERNEL_INT(uint64_t, 2);
        DEFINE_ROUNDING_KERNEL_INT(int64_t, 2);

        template <>
        struct rounding_kernel<batch<float, 4>>
        {
            using batch_type = batch<float, 4>;

            static inline batch_type ceil(const batch_type& x)
            {
                return vrndpq_f32(x);
            }

            static inline batch_type floor(const batch_type& x)
            {
                return vrndmq_f32(x);
            }

            static inline batch_type trunc(const batch_type& x)
            {
                return vrndq_f32(x);
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                return vrndxq_f32(x);
            }
        };
    #endif

    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        template <>
        struct rounding_kernel<batch<double, 2>>
        {
            using batch_type = batch<double, 2>;
        
            static inline batch_type ceil(const batch_type& x)
            {
                return vrndpq_f64(x);
            }

            static inline batch_type floor(const batch_type& x)
            {
                return vrndmq_f64(x);
            }

            static inline batch_type trunc(const batch_type& x)
            {
                return vrndq_f64(x);
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                return vrndxq_f64(x);
            }
        };
    #endif

        /***************************
         * Fallback implementation *
         ***************************/

    #if defined(XSIMD_ENABLE_FALLBACK)

        template <class T, std::size_t N>
        struct rounding_kernel<batch<T, N>>
        {
            using batch_type = batch<T, N>;

            static inline batch_type ceil(const batch_type& x)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::ceil, x)
            }

            static inline batch_type floor(const batch_type& x)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::floor, x)
            }

            static inline batch_type trunc(const batch_type& x)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::trunc, x)
            }

            static inline batch_type nearbyint(const batch_type& x)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::nearbyint, x)
            }
        };
    #endif

        /**************************
         * Generic implementation *
         **************************/

        template <class B, bool = std::is_integral<typename B::value_type>::value>
        struct round_impl;

        template <class T, std::size_t N>
        struct round_impl<batch<T, N>, false>
        {
            using batch_type = batch<T, N>;

            static inline batch_type round(const batch_type& x)
            {
                batch_type v = abs(x);
                batch_type c = ceil(v);
                batch_type cp = select(c - batch_type(0.5) > v, c - batch_type(1), c);
                return select(v > maxflint<batch_type>(), x, copysign(cp, x));
            }
        };

        template <class T, size_t N>
        struct round_impl<batch<T, N>, true>
        {
            using batch_type = batch<T, N>;

            static inline batch_type round(const batch_type& rhs)
            {
                return rhs;
            }
        };

        template <class T, std::size_t N>
        inline batch<T, N> rint(const batch<T, N>& x)
        {
            return nearbyint(x);
        }
    }


    template <class B>
    inline batch_type_t<B> ceil(const simd_base<B>& x)
    {
        return impl::rounding_kernel<B>::ceil(x());
    }

    template <class B>
    inline batch_type_t<B> floor(const simd_base<B>& x)
    {
        return impl::rounding_kernel<B>::floor(x());
    }

    template <class B>
    inline batch_type_t<B> trunc(const simd_base<B>& x)
    {
        return impl::rounding_kernel<B>::trunc(x());
    }

    template <class B>
    inline batch_type_t<B> round(const simd_base<B>& x)
    {
        return impl::round_impl<B>::round(x());
    }

    // Contrary to their std counterpart, these functions
    // are assume that the rounding mode is FE_TONEAREST
    template <class B>
    inline batch_type_t<B> nearbyint(const simd_base<B>& x)
    {
        return impl::rounding_kernel<B>::nearbyint(x());
    }

    template <class B>
    inline batch_type_t<B> rint(const simd_base<B>& x)
    {
        return impl::rint(x());
    }

}

#endif
