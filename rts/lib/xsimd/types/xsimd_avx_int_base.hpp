/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_INT_BASE_HPP
#define XSIMD_AVX_INT_BASE_HPP

#include "xsimd_base.hpp"

namespace xsimd
{

#if XSIMD_X86_INSTR_SET < XSIMD_X86_AVX512_VERSION

#define XSIMD_SPLIT_AVX(avx_name)                              \
    __m128i avx_name##_low = _mm256_castsi256_si128(avx_name); \
    __m128i avx_name##_high = _mm256_extractf128_si256(avx_name, 1)

#define XSIMD_RETURN_MERGED_SSE(res_low, res_high)    \
    __m256i result = _mm256_castsi128_si256(res_low); \
    return _mm256_insertf128_si256(result, res_high, 1)

#define XSIMD_APPLY_SSE_FUNCTION(func, avx_lhs, avx_rhs)     \
    XSIMD_SPLIT_AVX(avx_lhs);                                \
    XSIMD_SPLIT_AVX(avx_rhs);                                \
    __m128i res_low = func(avx_lhs##_low, avx_rhs##_low);    \
    __m128i res_high = func(avx_lhs##_high, avx_rhs##_high); \
    XSIMD_RETURN_MERGED_SSE(res_low, res_high);

#endif

    template <class T, std::size_t N>
    class avx_int_batch_bool : public simd_batch_bool<batch_bool<T, N>>
    {
    public:

        avx_int_batch_bool();
        explicit avx_int_batch_bool(bool b);
        template <class... Args, class Enable = detail::is_array_initializer_t<bool, N, Args...>>
        avx_int_batch_bool(Args... args);

        avx_int_batch_bool(const __m256i& rhs);
        avx_int_batch_bool& operator=(const __m256i& rhs);

        operator __m256i() const;

        bool_proxy<T> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        __m256i get_value() const;

    private:

        template <class... Args>
        batch_bool<T, N>& load_values(Args... args);

        union
        {
            __m256i m_value;
            T m_array[N];
        };

        friend class simd_batch_bool<batch_bool<T, N>>;
    };

    template <class T, std::size_t N>
    class avx_int_batch : public simd_batch<batch<T, N>>
    {
    public:

        using base_type = simd_batch<batch<T, N>>;
        using batch_bool_type = typename base_type::batch_bool_type;

        avx_int_batch();
        explicit avx_int_batch(T i);
        // Constructor from N scalar parameters
        template <class... Args, class Enable = detail::is_array_initializer_t<T, N, Args...>>
        avx_int_batch(Args... exactly_N_scalars);

        explicit avx_int_batch(const T* src);
        avx_int_batch(const T* src, aligned_mode);
        avx_int_batch(const T* src, unaligned_mode);
        avx_int_batch(const __m256i& rhs);
        avx_int_batch(const batch_bool_type& rhs);

        batch<T, N>& operator=(const __m256i& rhs);
        batch<T, N>& operator=(const batch_bool_type& rhs);

        operator __m256i() const;

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

    namespace avx_detail
    {
        template <class... Args>
        inline __m256i int_init(std::integral_constant<std::size_t, 1>, Args... args)
        {
            return _mm256_setr_epi8(args...);
        }

        template <class... Args>
        inline __m256i int_init(std::integral_constant<std::size_t, 2>, Args... args)
        {
            return _mm256_setr_epi16(args...);
        }

        template <class... Args>
        inline __m256i int_init(std::integral_constant<std::size_t, 4>, Args... args)
        {
            return _mm256_setr_epi32(args...);
        }

        template <class... Args>
        inline __m256i int_init(std::integral_constant<std::size_t, 8>, Args... args)
        {
            return _mm256_setr_epi64x(args...);
        }

        template <class T>
        inline __m256i int_set(std::integral_constant<std::size_t, 1>, T v)
        {
            return _mm256_set1_epi8(v);
        }

        template <class T>
        inline __m256i int_set(std::integral_constant<std::size_t, 2>, T v)
        {
            return _mm256_set1_epi16(v);
        }

        template <class T>
        inline __m256i int_set(std::integral_constant<std::size_t, 4>, T v)
        {
            return _mm256_set1_epi32(v);
        }

        template <class T>
        inline __m256i int_set(std::integral_constant<std::size_t, 8>, T v)
        {
            return _mm256_set1_epi64x(v);
        }
    }

    /*****************************************
     * batch_bool<T, N> implementation *
     *****************************************/

    template <class T, std::size_t N>
    inline avx_int_batch_bool<T, N>::avx_int_batch_bool()
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch_bool<T, N>::avx_int_batch_bool(bool b)
        : m_value(_mm256_set1_epi32(-(int32_t)b))
    {
    }

    template <class T, std::size_t N>
    template <class... Args, class>
    inline avx_int_batch_bool<T, N>::avx_int_batch_bool(Args... args)
        : m_value(avx_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{},
                                       static_cast<T>(args ? typename std::make_signed<T>::type{-1} : 0)...))
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch_bool<T, N>::avx_int_batch_bool(const __m256i& rhs)
        : m_value(rhs)
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch_bool<T, N>& avx_int_batch_bool<T, N>::operator=(const __m256i& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T, std::size_t N>
    inline avx_int_batch_bool<T, N>::operator __m256i() const
    {
        return m_value;
    }

    template <class T, std::size_t N>
    inline bool_proxy<T> avx_int_batch_bool<T, N>::operator[](std::size_t index)
    {
        return bool_proxy<T>(m_array[index & (N - 1)]);
    }

    template <class T, std::size_t N>
    inline bool avx_int_batch_bool<T, N>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & (N - 1)]);
    }

    template <class T, std::size_t N>
    inline __m256i avx_int_batch_bool<T, N>::get_value() const
    {
        return m_value;
    }

    template <class T, std::size_t N>
    template <class... Args>
    inline batch_bool<T, N>& avx_int_batch_bool<T, N>::load_values(Args... args)
    {
        m_value = avx_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{},
                                       static_cast<T>(args ? typename std::make_signed<T>::type{-1} : 0)...);
        return (*this)();
    }
    
    namespace detail
    {
        template <class T, std::size_t N>
        struct avx_int_batch_bool_kernel
        {
            using batch_type = batch_bool<T, N>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_and_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_and_si128, lhs, rhs);
#endif
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_or_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_or_si128, lhs, rhs);
#endif
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_xor_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_xor_si128, lhs, rhs);
#endif
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_xor_si256(rhs, _mm256_set1_epi32(-1)); // xor with all one
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_xor_si128(rhs_low, _mm_set1_epi32(-1));
                __m128i res_high = _mm_xor_si128(rhs_high, _mm_set1_epi32(-1));
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_andnot_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_andnot_si128, lhs, rhs);
#endif
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                switch(sizeof(T))
                {
                    case 1:
                        return _mm256_cmpeq_epi8(lhs, rhs);
                    case 2:
                        return _mm256_cmpeq_epi16(lhs, rhs);
                    case 4:
                        return _mm256_cmpeq_epi32(lhs, rhs);
                    case 8:
                        return _mm256_cmpeq_epi64(lhs, rhs);
                }
#else
                switch(sizeof(T))
                {
                    case 1:
                    {
                        XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi8, lhs, rhs);
                    }
                    case 2:
                    {
                        XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi16, lhs, rhs);
                    }
                    case 4:
                    {
                        XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi32, lhs, rhs);
                    }
                    case 8:
                    {
                        XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi64, lhs, rhs);
                    }
                }
#endif
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return ~(lhs == rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return _mm256_testc_si256(rhs, batch_type(true)) != 0;
            }

            static bool any(const batch_type& rhs)
            {
                return !_mm256_testz_si256(rhs, rhs);
            }
        };
    }

    /**************************************
     * avx_int_batch<T, N> implementation *
     **************************************/

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch()
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch(T i)
        : base_type(avx_detail::int_set(std::integral_constant<std::size_t, sizeof(T)>{}, i))
    {
    }

    template <class T, std::size_t N>
    template <class... Args, class>
    inline avx_int_batch<T, N>::avx_int_batch(Args... args)
        : base_type(avx_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{}, args...))
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch(const T* src)
        : base_type(_mm256_loadu_si256((__m256i const*)src))
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch(const T* src, aligned_mode)
        : base_type(_mm256_load_si256((__m256i const*)src))
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch(const T* src, unaligned_mode)
        : base_type(_mm256_loadu_si256((__m256i const*)src))
    {
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch(const __m256i& rhs)
        : base_type(rhs)
    {
    }

    namespace detail
    {
        inline __m256i bitwise_and_impl(__m256i lhs, __m256i rhs)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            return _mm256_and_si256(lhs, rhs);
#else
            XSIMD_APPLY_SSE_FUNCTION(_mm_and_si128, lhs, rhs);
#endif
        }
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::avx_int_batch(const batch_bool_type& rhs)
        : base_type(detail::bitwise_and_impl(rhs, batch<T, N>(1)))
    {
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx_int_batch<T, N>::operator=(const __m256i& rhs)
    {
        this->m_value = rhs;
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx_int_batch<T, N>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = detail::bitwise_and_impl(rhs, batch<T, N>(1));
        return (*this)();
    }

    template <class T, std::size_t N>
    inline avx_int_batch<T, N>::operator __m256i() const
    {
        return this->m_value;
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx_int_batch<T, N>::load_aligned(const T* src)
    {
        this->m_value = _mm256_load_si256((__m256i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx_int_batch<T, N>::load_unaligned(const T* src)
    {
        this->m_value = _mm256_loadu_si256((__m256i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx_int_batch<T, N>::load_aligned(const flipped_sign_type_t<T>* src)
    {
        this->m_value = _mm256_load_si256((__m256i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& avx_int_batch<T, N>::load_unaligned(const flipped_sign_type_t<T>* src)
    {
        this->m_value = _mm256_loadu_si256((__m256i const*) src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline void avx_int_batch<T, N>::store_aligned(T* dst) const
    {
        _mm256_store_si256((__m256i*) dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void avx_int_batch<T, N>::store_unaligned(T* dst) const
    {
        _mm256_storeu_si256((__m256i*) dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void avx_int_batch<T, N>::store_aligned(flipped_sign_type_t<T>* dst) const
    {
        _mm256_store_si256((__m256i*) dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void avx_int_batch<T, N>::store_unaligned(flipped_sign_type_t<T>* dst) const
    {
        _mm256_storeu_si256((__m256i*) dst, this->m_value);
    }

    namespace detail
    {
        template <class B>
        struct avx_int_kernel_base
        {
            using batch_type = B;
            using batch_bool_type = typename simd_batch_traits<B>::batch_bool_type;
            // static constexpr std::size_t size = simd_batch_traits<B>::size;
            // static constexpr std::size_t align = simd_batch_traits<B>::align;

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return ~(lhs == rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return ~(rhs < lhs);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return detail::bitwise_and_impl(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_or_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_or_si128, lhs, rhs);
#endif
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_xor_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_xor_si128, lhs, rhs);
#endif
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_xor_si256(rhs, _mm256_set1_epi8(-1));
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_xor_si128(rhs_low, _mm_set1_epi8(-1));
                __m128i res_high = _mm_xor_si128(rhs_high, _mm_set1_epi8(-1));
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_andnot_si256(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_andnot_si128, lhs, rhs);
#endif
            }

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

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return x * y + z;
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return x * y - z;
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return -x * y + z;
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return -x * y - z;
            }
        };
    }

    namespace avx_detail
    {
        template <class F, class T, std::size_t N>
        inline batch<T, N> shift_impl(F&& f, const batch<T, N>& lhs, int32_t rhs)
        {
            alignas(32) T tmp_lhs[N], tmp_res[N];
            lhs.store_aligned(&tmp_lhs[0]);
            unroller<N>([&](std::size_t i) {
                tmp_res[i] = f(tmp_lhs[i], rhs);
            });
            return batch<T, N>(tmp_res, aligned_mode());
        }

        template <class F, class T, class S, std::size_t N>
        inline batch<T, N> shift_impl(F&& f, const batch<T, N>& lhs, const batch<S, N>& rhs)
        {
            alignas(32) T tmp_lhs[N], tmp_res[N];
            alignas(32) S tmp_rhs[N];
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
