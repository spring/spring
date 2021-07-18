/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_INT_BASE_HPP
#define XSIMD_SSE_INT_BASE_HPP

#include <cstdint>

#include "xsimd_base.hpp"

namespace xsimd
{

   /********************
     * batch_bool<T, N> *
     ********************/

    template <class T, std::size_t N>
    class sse_int_batch_bool : public simd_batch_bool<batch_bool<T, N>>
    {
    public:

        sse_int_batch_bool();
        explicit sse_int_batch_bool(bool b);
        template <class... Args, class Enable = detail::is_array_initializer_t<bool, N, Args...>>
        sse_int_batch_bool(Args... args);
        sse_int_batch_bool(const __m128i& rhs);
        sse_int_batch_bool& operator=(const __m128i& rhs);

        operator __m128i() const;

        bool_proxy<T> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        __m128i get_value() const;

    private:

        template <class... Args>
        batch_bool<T, N>& load_values(Args... args);

        union
        {
            __m128i m_value;
            T m_array[N];
        };

        friend class simd_batch_bool<batch_bool<T, N>>;
    };

    /***********************
     * sse_int_batch<T, N> *
     ***********************/

    template <class T, std::size_t N>
    class sse_int_batch : public simd_batch<batch<T, N>>
    {
    public:

        using base_type = simd_batch<batch<T, N>>;
        using batch_bool_type = typename base_type::batch_bool_type;

        sse_int_batch();
        explicit sse_int_batch(T i);
        template <class... Args, class Enable = detail::is_array_initializer_t<T, N, Args...>>
        constexpr sse_int_batch(Args... args);
        explicit sse_int_batch(const T* src);
        sse_int_batch(const T* src, aligned_mode);
        sse_int_batch(const T* src, unaligned_mode);
        sse_int_batch(const __m128i& rhs);
        sse_int_batch(const batch_bool_type& rhs);

        batch<T, N>& operator=(const __m128i& rhs);
        batch<T, N>& operator=(const batch_bool_type& rhs);

        operator __m128i() const;

        batch<T, N>& load_aligned(const T* src);
        batch<T, N>& load_unaligned(const T* src);

        batch<T, N>& load_aligned(const flipped_sign_type_t<T>* src);
        batch<T, N>& load_unaligned(const flipped_sign_type_t<T>* src);

        void store_aligned(T* dst) const;
        void store_unaligned(T* dst) const;

        void store_aligned(flipped_sign_type_t<T>* src) const;
        void store_unaligned(flipped_sign_type_t<T>* src) const;

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /********************
     * helper functions *
     ********************/

    namespace sse_detail
    {
        template <class... Args>
        inline __m128i int_init(std::integral_constant<std::size_t, 1>, Args... args)
        {
            return _mm_setr_epi8(args...);
        }

        template <class... Args>
        inline __m128i int_init(std::integral_constant<std::size_t, 2>, Args... args)
        {
            return _mm_setr_epi16(args...);
        }

        template <class... Args>
        inline __m128i int_init(std::integral_constant<std::size_t, 4>, Args... args)
        {
            return _mm_setr_epi32(args...);
        }

        template <class... Args, class I0, class I1>
        inline __m128i int_init(std::integral_constant<std::size_t, 8>, I0 i0, I1 i1)
        {
            return _mm_set_epi64x(i1, i0);
        }

        template <class T>
        inline __m128i int_set(std::integral_constant<std::size_t, 1>, T v)
        {
            return _mm_set1_epi8(v);
        }

        template <class T>
        inline __m128i int_set(std::integral_constant<std::size_t, 2>, T v)
        {
            return _mm_set1_epi16(v);
        }

        template <class T>
        inline __m128i int_set(std::integral_constant<std::size_t, 4>, T v)
        {
            return _mm_set1_epi32(v);
        }

        template <class T>
        inline __m128i int_set(std::integral_constant<std::size_t, 8>, T v)
        {
            return _mm_set1_epi64x(v);
        }

        inline __m128i cmpeq_epi64_sse2(__m128i lhs, __m128i rhs)
        {
            __m128i tmp1 = _mm_cmpeq_epi32(lhs, rhs);
            __m128i tmp2 = _mm_shuffle_epi32(tmp1, 0xB1);
            __m128i tmp3 = _mm_and_si128(tmp1, tmp2);
            __m128i tmp4 = _mm_srai_epi32(tmp3, 31);
            return _mm_shuffle_epi32(tmp4, 0xF5);
        }
    }

    /***********************************
     * batch_bool<T, N> implementation *
     ***********************************/

    template <class T, std::size_t N>
    inline sse_int_batch_bool<T, N>::sse_int_batch_bool()
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch_bool<T, N>::sse_int_batch_bool(bool b)
        : m_value(_mm_set1_epi32(-(int)b))
    {
    }

    template <class T, std::size_t N>
    template <class... Args, class>
    inline sse_int_batch_bool<T, N>::sse_int_batch_bool(Args... args)
        : m_value(sse_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{},
                                       static_cast<T>(args ? typename std::make_signed<T>::type{-1} : 0)...))
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch_bool<T, N>::sse_int_batch_bool(const __m128i& rhs)
        : m_value(rhs)
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch_bool<T, N>& sse_int_batch_bool<T, N>::operator=(const __m128i& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T, std::size_t N>
    inline sse_int_batch_bool<T, N>::operator __m128i() const
    {
        return m_value;
    }

    template <class T, std::size_t N>
    inline bool_proxy<T> sse_int_batch_bool<T, N>::operator[](std::size_t index)
    {
        return bool_proxy<T>(m_array[index & (N - 1)]);
    }

    template <class T, std::size_t N>
    inline bool sse_int_batch_bool<T, N>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & (N - 1)]);
    }

    template <class T, std::size_t N>
    inline __m128i sse_int_batch_bool<T, N>::get_value() const
    {
        return m_value;
    }
    
    template <class T, std::size_t N>
    template <class... Args>
    inline batch_bool<T, N>& sse_int_batch_bool<T, N>::load_values(Args... args)
    {
        m_value = sse_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{},
                                       static_cast<T>(args ? typename std::make_signed<T>::type{-1} : 0)...);
        return (*this)();
    }
    
    namespace detail
    {
        template <class T>
        struct sse_int_batch_bool_kernel
        {
            using batch_type = batch_bool<T, 128 / (sizeof(T) * 8)>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_and_si128(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_or_si128(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_xor_si128(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm_xor_si128(rhs, _mm_set1_epi32(-1));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_andnot_si128(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                switch(sizeof(T))
                {
                    case 1:
                        return _mm_cmpeq_epi8(lhs, rhs);
                    case 2:
                        return _mm_cmpeq_epi16(lhs, rhs);
                    case 4:
                        return _mm_cmpeq_epi32(lhs, rhs);
                    case 8:
                    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                        return _mm_cmpeq_epi64(lhs, rhs);
#else
                        return sse_detail::cmpeq_epi64_sse2(lhs, rhs);
#endif
                    }
                }
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return ~(lhs == rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return _mm_movemask_epi8(rhs) == 0xFFFF;
            }

            static bool any(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return !_mm_testz_si128(rhs, rhs);
#else
                return _mm_movemask_epi8(rhs) != 0;
#endif
            }
        };
    }

    /**************************************
     * sse_int_batch<T, N> implementation *
     **************************************/

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch()
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch(T i)
        : base_type(sse_detail::int_set(std::integral_constant<std::size_t, sizeof(T)>{}, i))
    {
    }

    template <class T, std::size_t N>
    template <class... Args, class>
    constexpr inline sse_int_batch<T, N>::sse_int_batch(Args... args)
        : base_type(sse_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{}, args...))
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch(const T* src)
        : base_type(_mm_loadu_si128((__m128i const*)src))
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch(const T* src, aligned_mode)
        : base_type(_mm_load_si128((__m128i const*)src))
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch(const T* src, unaligned_mode)
        : base_type(_mm_loadu_si128((__m128i const*)src))
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch(const __m128i& rhs)
        : base_type(rhs)
    {
    }

    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::sse_int_batch(const batch_bool_type& rhs)
        : base_type(_mm_and_si128(rhs, batch<T, N>(1)))
    {
    }

    template <class T, std::size_t N>
    inline batch<T, N>& sse_int_batch<T, N>::operator=(const __m128i& rhs)
    {
        this->m_value = rhs;
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& sse_int_batch<T, N>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = _mm_and_si128(rhs, batch<T, N>(1));
        return (*this)();
    }
    
    template <class T, std::size_t N>
    inline sse_int_batch<T, N>::operator __m128i() const
    {
        return this->m_value;
    }

    template <class T, std::size_t N>
    inline batch<T, N>& sse_int_batch<T, N>::load_aligned(const T* src)
    {
        this->m_value = _mm_load_si128((__m128i const*)src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& sse_int_batch<T, N>::load_unaligned(const T* src)
    {
        this->m_value = _mm_loadu_si128((__m128i const*)src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& sse_int_batch<T, N>::load_aligned(const flipped_sign_type_t<T>* src)
    {
        this->m_value = _mm_load_si128((__m128i const*)src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline batch<T, N>& sse_int_batch<T, N>::load_unaligned(const flipped_sign_type_t<T>* src)
    {
        this->m_value = _mm_loadu_si128((__m128i const*)src);
        return (*this)();
    }

    template <class T, std::size_t N>
    inline void sse_int_batch<T, N>::store_aligned(T* dst) const
    {
        _mm_store_si128((__m128i*)dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void sse_int_batch<T, N>::store_unaligned(T* dst) const
    {
        _mm_storeu_si128((__m128i*)dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void sse_int_batch<T, N>::store_aligned(flipped_sign_type_t<T>* dst) const
    {
        _mm_store_si128((__m128i*)dst, this->m_value);
    }

    template <class T, std::size_t N>
    inline void sse_int_batch<T, N>::store_unaligned(flipped_sign_type_t<T>* dst) const
    {
        _mm_storeu_si128((__m128i*)dst, this->m_value);
    }

    namespace detail
    {
        template <class B>
        struct sse_int_kernel_base
        {
            using batch_type = B;
            using batch_bool_type = typename simd_batch_traits<B>::batch_bool_type;
            static constexpr std::size_t size = simd_batch_traits<B>::size;
            static constexpr std::size_t align = simd_batch_traits<B>::align;

            static batch_type neg(const batch_type& rhs)
            {
                return batch_type(_mm_setzero_si128()) - rhs;
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                // TODO implement divison as floating point!
                XSIMD_MACRO_UNROLL_BINARY(/);
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(%);
            }

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
                return _mm_and_si128(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_or_si128(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_xor_si128(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm_xor_si128(rhs, _mm_set1_epi8(-1));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_andnot_si128(lhs, rhs);
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

    namespace sse_detail
    {
        template <class F, class T, std::size_t N>
        inline batch<T, N> shift_impl(F&& f, const batch<T, N>& lhs, int32_t rhs)
        {
            alignas(16) T tmp_lhs[N], tmp_res[N];
            lhs.store_aligned(&tmp_lhs[0]);
            unroller<N>([&](std::size_t i) {
                tmp_res[i] = f(tmp_lhs[i], rhs);
            });
            return batch<T, N>(tmp_res, aligned_mode());
        }

        template <class F, class T, class S, std::size_t N>
        inline batch<T, N> shift_impl(F&& f, const batch<T, N>& lhs, const batch<S, N>& rhs)
        {
            alignas(16) T tmp_lhs[N], tmp_res[N];
            alignas(16) S tmp_rhs[N];
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
