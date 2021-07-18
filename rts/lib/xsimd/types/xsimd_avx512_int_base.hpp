/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_INT512_BASE_HPP
#define XSIMD_AVX_INT512_BASE_HPP

#include "xsimd_base.hpp"
#include "xsimd_utils.hpp"

namespace xsimd
{

#define XSIMD_SPLIT_AVX512(avx_name)                                                                  \
    __m256i avx_name##_low = _mm512_castsi512_si256((__m512i)avx_name);                                        \
    __m256i avx_name##_high = _mm512_extracti64x4_epi64((__m512i)avx_name, 1)                                  \

#define XSIMD_SPLITPS_AVX512(avx_name)                                                                  \
    __m256 avx_name##_low = _mm512_castps512_ps256((__m512)avx_name);                                        \
    __m256 avx_name##_high = _mm512_extractf32x8_ps((__m512)avx_name, 1)                                  \

#define XSIMD_SPLITPD_AVX512(avx_name)                                                                  \
    __m256d avx_name##_low = _mm512_castpd512_pd256((__m512d)avx_name);                                        \
    __m256d avx_name##_high = _mm512_extractf64x4_pd((__m512d)avx_name, 1)                                  \

#define XSIMD_RETURN_MERGED_AVX(res_low, res_high)                                                    \
    __m512i result = _mm512_castsi256_si512(res_low);                                                 \
    return _mm512_inserti64x4(result, res_high, 1)                                                    \

#define XSIMD_RETURN_MERGEDPS_AVX(res_low, res_high)                                                    \
    __m512 result = _mm512_castps256_ps512(res_low);                                                 \
    return _mm512_insertf32x8(result, res_high, 1)                                                    \

#define XSIMD_RETURN_MERGEDPD_AVX(res_low, res_high)                                                    \
    __m512d result = _mm512_castpd256_pd512(res_low);                                                 \
    return _mm512_insertf64x4(result, res_high, 1)                                                    \

#define XSIMD_APPLY_AVX2_FUNCTION(N, func, avx_lhs, avx_rhs)                                          \
    XSIMD_SPLIT_AVX512(avx_lhs);                                                                      \
    XSIMD_SPLIT_AVX512(avx_rhs);                                                                      \
    __m256i res_low = detail::batch_kernel<value_type, N> :: func (avx_lhs##_low, avx_rhs##_low);     \
    __m256i res_high = detail::batch_kernel<value_type, N> :: func (avx_lhs##_high, avx_rhs##_high);  \
    XSIMD_RETURN_MERGED_AVX(res_low, res_high);

    namespace detail
    {
        template <std::size_t N>
        struct mask_type;

        template <>
        struct mask_type<8>
        {
            using type = __mmask8;
        };

        template <>
        struct mask_type<16>
        {
            using type = __mmask16;
        };

        template <>
        struct mask_type<32>
        {
            using type = __mmask32;
        };

        template <>
        struct mask_type<64>
        {
            using type = __mmask64;
        };

        template <std::size_t N>
        using mask_type_t = typename mask_type<N>::type;
    }

    template <class T, std::size_t N>
    class avx512_int_batch : public simd_batch<batch<T, N>>
    {
    public:

        using base_type = simd_batch<batch<T, N>>;
        using mask_type = detail::mask_type_t<N>;

        avx512_int_batch();
        explicit avx512_int_batch(T i);

        template <class... Args, class Enable = detail::is_array_initializer_t<T, N, Args...>>
        avx512_int_batch(Args... exactly_N_scalars);
        explicit avx512_int_batch(const T* src);
        avx512_int_batch(const T* src, aligned_mode);
        avx512_int_batch(const T* src, unaligned_mode);

        avx512_int_batch(const __m512i& rhs);
        avx512_int_batch& operator=(const __m512i& rhs);

        avx512_int_batch(const batch_bool<T, N>& rhs);
        avx512_int_batch& operator=(const batch_bool<T, N>& rhs);

        operator __m512i() const;

        batch<T, N>& load_aligned(const T* src);
        batch<T, N>& load_unaligned(const T* src);

        batch<T, N>& load_aligned(const flipped_sign_type_t<T>* src);
        batch<T, N>& load_unaligned(const flipped_sign_type_t<T>* src);

        void store_aligned(T* dst) const;
        void store_unaligned(T* dst) const;

        void store_aligned(flipped_sign_type_t<T>* dst) const;
        void store_unaligned(flipped_sign_type_t<T>* dst) const;

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /***********************************
     * avx512_int_batch implementation *
     ***********************************/

    namespace avx512_detail
    {
        inline __m512i int_init(std::integral_constant<std::size_t, 1>,
                         int8_t t0, int8_t t1, int8_t t2, int8_t t3,
                         int8_t t4, int8_t t5, int8_t t6, int8_t t7,
                         int8_t t8, int8_t t9, int8_t t10, int8_t t11,
                         int8_t t12, int8_t t13, int8_t t14, int8_t t15,
                         int8_t t16, int8_t t17, int8_t t18, int8_t t19,
                         int8_t t20, int8_t t21, int8_t t22, int8_t t23,
                         int8_t t24, int8_t t25, int8_t t26, int8_t t27,
                         int8_t t28, int8_t t29, int8_t t30, int8_t t31,
                         int8_t t32, int8_t t33, int8_t t34, int8_t t35,
                         int8_t t36, int8_t t37, int8_t t38, int8_t t39,
                         int8_t t40, int8_t t41, int8_t t42, int8_t t43,
                         int8_t t44, int8_t t45, int8_t t46, int8_t t47,
                         int8_t t48, int8_t t49, int8_t t50, int8_t t51,
                         int8_t t52, int8_t t53, int8_t t54, int8_t t55,
                         int8_t t56, int8_t t57, int8_t t58, int8_t t59,
                         int8_t t60, int8_t t61, int8_t t62, int8_t t63)
        {
#if defined(__clang__) || __GNUC__
            return __extension__ (__m512i)(__v64qi)
            {
              t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
              t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29, t30, t31,
              t32, t33, t34, t35, t36, t37, t38, t39, t40, t41, t42, t43, t44, t45, t46, t47,
              t48, t49, t50, t51, t52, t53, t54, t55, t56, t57, t58, t59, t60, t61, t62, t63
            };
#else
            return _mm512_set_epi8(
              t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
              t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29, t30, t31,
              t32, t33, t34, t35, t36, t37, t38, t39, t40, t41, t42, t43, t44, t45, t46, t47,
              t48, t49, t50, t51, t52, t53, t54, t55, t56, t57, t58, t59, t60, t61, t62, t63);
#endif
        }

        inline __m512i int_init(std::integral_constant<std::size_t, 2>,
                         int16_t t0, int16_t t1, int16_t t2, int16_t t3,
                         int16_t t4, int16_t t5, int16_t t6, int16_t t7,
                         int16_t t8, int16_t t9, int16_t t10, int16_t t11,
                         int16_t t12, int16_t t13, int16_t t14, int16_t t15,
                         int16_t t16, int16_t t17, int16_t t18, int16_t t19,
                         int16_t t20, int16_t t21, int16_t t22, int16_t t23,
                         int16_t t24, int16_t t25, int16_t t26, int16_t t27,
                         int16_t t28, int16_t t29, int16_t t30, int16_t t31)
        {
#if defined(__clang__) || __GNUC__
            return __extension__ (__m512i)(__v32hi)
            {
              t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
              t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29, t30, t31
            };
#else
            return _mm512_set_epi16(
              t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
              t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29, t30, t31);
#endif
        }

        inline __m512i int_init(std::integral_constant<std::size_t, 4>,
                                int32_t t0, int32_t t1, int32_t t2, int32_t t3,
                                int32_t t4, int32_t t5, int32_t t6, int32_t t7,
                                int32_t t8, int32_t t9, int32_t t10, int32_t t11,
                                int32_t t12, int32_t t13, int32_t t14, int32_t t15)
        {
            // _mm512_setr_epi32 is a macro, preventing parameter pack expansion ...
            return _mm512_setr_epi32(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15);
        }

        inline __m512i int_init(std::integral_constant<std::size_t, 8>,
                                int64_t t0, int64_t t1, int64_t t2, int64_t t3,
                                int64_t t4, int64_t t5, int64_t t6, int64_t t7)
        {
            // _mm512_setr_epi64 is a macro, preventing parameter pack expansion ...
            return _mm512_setr_epi64(t0, t1, t2, t3, t4, t5, t6, t7);
        }

        template <class T>
        inline __m512i int_set(std::integral_constant<std::size_t, 1>, T v)
        {
            return _mm512_set1_epi8(v);
        }

        template <class T>
        inline __m512i int_set(std::integral_constant<std::size_t, 2>, T v)
        {
            return _mm512_set1_epi16(v);
        }

        template <class T>
        inline __m512i int_set(std::integral_constant<std::size_t, 4>, T v)
        {
            return _mm512_set1_epi32(v);
        }

        template <class T>
        inline __m512i int_set(std::integral_constant<std::size_t, 8>, T v)
        {
            return _mm512_set1_epi64(v);
        }
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch()
    {
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch(T i)
        : base_type(avx512_detail::int_set(std::integral_constant<std::size_t, sizeof(T)>{}, i))
    {
    }

    template <class T, std::size_t N>
    template <class... Args, class>
    inline avx512_int_batch<T, N>::avx512_int_batch(Args... args)
        : base_type(avx512_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{}, args...))
    {
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch(const T* src)
        : base_type(_mm512_loadu_si512((__m512i const*) src))
    {
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch(const T* src, aligned_mode)
        : base_type(_mm512_load_si512((__m512i const*) src))
    {
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch(const T* src, unaligned_mode)
        : base_type(_mm512_loadu_si512((__m512i const*) src))
    {
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch(const __m512i& rhs)
        : base_type(rhs)
    {
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>& avx512_int_batch<T, N>::operator=(const __m512i& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::avx512_int_batch(const batch_bool<T, N>& rhs)
        :   base_type(detail::batch_kernel<T, N>::select(rhs, batch<T, N>(T(1)), batch<T, N>(T(0))))
    {
    }

    template <class T, std::size_t N>
    avx512_int_batch<T, N>& avx512_int_batch<T, N>::operator=(const batch_bool<T, N>& rhs)
    {
        this->m_value = detail::batch_kernel<T, N>::select(rhs, batch<T, N>(T(1)), batch<T, N>(T(0)));
        return *this;
    }
    
    template <class T, std::size_t N>
    inline avx512_int_batch<T, N>::operator __m512i() const
    {
        return this->m_value;
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx512_int_batch<T, N>::load_aligned(const T* src)
    {
        this->m_value = _mm512_load_si512((__m512i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx512_int_batch<T, N>::load_unaligned(const T* src)
    {
        this->m_value = _mm512_loadu_si512((__m512i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx512_int_batch<T, N>::load_aligned(const flipped_sign_type_t<T>* src)
    {
        this->m_value = _mm512_load_si512((__m512i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx512_int_batch<T, N>::load_unaligned(const flipped_sign_type_t<T>* src)
    {
        this->m_value = _mm512_loadu_si512((__m512i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline void avx512_int_batch<T, N>::store_aligned(T* dst) const
    {
        _mm512_store_si512(dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void avx512_int_batch<T, N>::store_unaligned(T* dst) const
    {
        _mm512_storeu_si512(dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void avx512_int_batch<T, N>::store_aligned(flipped_sign_type_t<T>* dst) const
    {
        _mm512_store_si512(dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void avx512_int_batch<T, N>::store_unaligned(flipped_sign_type_t<T>* dst) const
    {
        _mm512_storeu_si512(dst, this->m_value);
    }

    namespace detail
    {
        template <class B>
        struct avx512_int_kernel_base
        {
            using batch_type = B;

            static batch_type fmin(const batch_type& lhs, const batch_type& rhs)
            {
                return min(lhs, rhs);
            }

            static batch_type fmax(const batch_type& lhs, const batch_type& rhs)
            {
                return max(lhs, rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }
        };
    }

    namespace avx512_detail
    {
        template <class F, class T, std::size_t N>
        inline batch<T, N> shift_impl(F&& f, const batch<T, N>& lhs, int32_t rhs)
        {
            alignas(64) T tmp_lhs[N], tmp_res[N];
            lhs.store_aligned(&tmp_lhs[0]);
            unroller<N>([&](std::size_t i) {
                tmp_res[i] = f(tmp_lhs[i], rhs);
            });
            return batch<T, N>(tmp_res, aligned_mode());
        }

        template <class F, class T, class S, std::size_t N>
        inline batch<T, N> shift_impl(F&& f, const batch<T, N>& lhs, const batch<S, N>& rhs)
        {
            alignas(64) T tmp_lhs[N], tmp_res[N];
            alignas(64) S tmp_rhs[N];
            lhs.store_aligned(&tmp_lhs[0]);
            rhs.store_aligned(&tmp_rhs[0]);
            unroller<N>([&](std::size_t i) {
              tmp_res[i] = f(tmp_lhs[i], tmp_rhs[i]);
            });
            return batch<T, N>(tmp_res, aligned_mode());
        }
    }
}

#endif
