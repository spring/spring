/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_BOOL_HPP
#define XSIMD_AVX512_BOOL_HPP

#include "xsimd_avx512_int_base.hpp"
#include "xsimd_utils.hpp"

namespace xsimd
{

    /*******************
     * bool_mask_proxy *
     *******************/

    template <class MASK>
    class bool_mask_proxy
    {
    public:

        bool_mask_proxy(MASK& ref, std::size_t idx);

        bool_mask_proxy(const bool_mask_proxy&) = default;
        bool_mask_proxy& operator=(const bool_mask_proxy&) = default;

        bool_mask_proxy(bool_mask_proxy&&) = default;
        bool_mask_proxy& operator=(bool_mask_proxy&&) = default;

        operator bool() const;
        bool_mask_proxy& operator=(bool);

    private:

        MASK& m_ref;
        std::size_t m_idx;
    };

    /*********************
     * batch_bool_avx512 *
     *********************/

    template <class MASK, class B>
    class batch_bool_avx512 : public simd_batch_bool<B>
    {
    public:

        batch_bool_avx512();
        explicit batch_bool_avx512(bool b);
        template <class... Args, class Enable = detail::is_array_initializer_t<bool, sizeof(MASK) * 8, Args...>>
        batch_bool_avx512(Args... args);
        batch_bool_avx512(const bool (&init)[sizeof(MASK) * 8]);

        batch_bool_avx512(const MASK& rhs);
        batch_bool_avx512& operator=(const __m512& rhs);

        bool_mask_proxy<MASK> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        operator MASK() const;

    private:

        B& load_array(const std::array<bool, sizeof(MASK) * 8>& src);

        template <class... Args>
        B& load_values(Args... args);

        MASK m_value;

        friend class simd_batch_bool<B>;
    };

    /******************************
     * avx512_fallback_batch_bool *
     ******************************/

    template <class T, std::size_t N>
    class avx512_fallback_batch_bool : public simd_batch_bool<batch_bool<T, N>>
    {
    public:

        avx512_fallback_batch_bool();
        explicit avx512_fallback_batch_bool(bool b);
        template <class... Args, class Enable = detail::is_array_initializer_t<bool, N, Args...>>
        avx512_fallback_batch_bool(Args... args);

        avx512_fallback_batch_bool(const __m512i& rhs);
        avx512_fallback_batch_bool& operator=(const __m512i& rhs);

        operator __m512i() const;

        bool_proxy<T> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

    private:

        template <class... Args>
        batch_bool<T, N>& load_values(Args... b);

        union
        {
            __m512i m_value;
            T m_array[N];
        };

        friend class simd_batch_bool<batch_bool<T, N>>;
    };

    /**********************************
     * bool_mask_proxy implementation *
     **********************************/

    template <class MASK>
    inline bool_mask_proxy<MASK>::bool_mask_proxy(MASK& ref, std::size_t idx)
        : m_ref(ref), m_idx(idx)
    {
    }

    template <class MASK>
    inline bool_mask_proxy<MASK>::operator bool() const
    {
        return ((m_ref >> m_idx) & MASK(1)) != 0;
    }

    template <class MASK>
    inline bool_mask_proxy<MASK>& bool_mask_proxy<MASK>::operator=(bool rhs)
    {
        MASK tmp = static_cast<MASK>(rhs);
        m_ref ^= (-tmp ^ m_ref) & (MASK(1) << m_idx);
        return *this;
    }

    /************************************
     * batch_bool_avx512 implementation *
     ************************************/

    template <class MASK, class T>
    inline batch_bool_avx512<MASK, T>::batch_bool_avx512()
    {
    }

    template <class MASK, class T>
    template <class... Args, class>
    inline batch_bool_avx512<MASK, T>::batch_bool_avx512(Args... args)
        : batch_bool_avx512({{static_cast<bool>(args)...}})
    {
    }

    template <class MASK, class T>
    inline batch_bool_avx512<MASK, T>::batch_bool_avx512(bool b)
        : m_value(b ? -1 : 0)
    {
    }

    namespace detail
    {
        template <class T>
        constexpr T get_init_value_impl(const bool (&/*init*/)[sizeof(T) * 8])
        {
            return T(0);
        }

        template <class T, std::size_t IX, std::size_t... I>
        constexpr T get_init_value_impl(const bool (&init)[sizeof(T) * 8])
        {
            return (T(init[IX]) << IX) | get_init_value_impl<T, I...>(init);
        }

        template <class T, std::size_t... I>
        constexpr T get_init_value(const bool (&init)[sizeof(T) * 8], detail::index_sequence<I...>)
        {
            return get_init_value_impl<T, I...>(init);
        }
    }

    template <class MASK, class T>
    inline batch_bool_avx512<MASK, T>::batch_bool_avx512(const bool (&init)[sizeof(MASK) * 8])
        : m_value(detail::get_init_value<MASK>(init, detail::make_index_sequence<sizeof(MASK) * 8>{}))
    {
    }

    template <class MASK, class T>
    inline batch_bool_avx512<MASK, T>::batch_bool_avx512(const MASK& rhs)
        : m_value(rhs)
    {
    }

    template <class MASK, class T>
    inline batch_bool_avx512<MASK, T>::operator MASK() const
    {
        return m_value;
    }

    template <class MASK, class T>
    inline bool_mask_proxy<MASK> batch_bool_avx512<MASK, T>::operator[](std::size_t idx)
    {
        std::size_t s = simd_batch_traits<T>::size - 1;
        return bool_mask_proxy<MASK>(m_value, idx & s);
    }

    template <class MASK, class T>
    inline bool batch_bool_avx512<MASK, T>::operator[](std::size_t idx) const
    {
        std::size_t s = simd_batch_traits<T>::size - 1;
        return (m_value & (MASK(1) << (idx & s))) != 0;
    }
    
    template <class MASK, class T>
    inline T& batch_bool_avx512<MASK, T>::load_array(const std::array<bool, sizeof(MASK) * 8>& src)
    {
        MASK tmp(false);
        for(std::size_t i = 0; i  < sizeof(MASK) * 8; ++i)
        {
            tmp |= MASK(src[i]) << i;
        }
        m_value = tmp;
        return (*this)();
    }

    template <class MASK, class T>
    template <class... Args>
    inline T& batch_bool_avx512<MASK, T>::load_values(Args... b)
    {
        return load_array({{b...}});
    }
    
    namespace detail
    {
        template <class T, std::size_t N>
        struct batch_bool_kernel_avx512
        {
            using batch_type = batch_bool<T, N>;
            using mt = typename mask_type<N>::type;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return mt(lhs) & mt(rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return mt(lhs) | mt(rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return mt(lhs) ^ mt(rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return ~mt(rhs);
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return mt(lhs) ^ mt(rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return (~mt(lhs)) ^ mt(rhs);
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return mt(lhs) ^ mt(rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return mt(rhs) == mt(-1);
            }

            static bool any(const batch_type& rhs)
            {
                return mt(rhs) != mt(0);
            }
        };
    }

    /*********************************************
     * avx512_fallback_batch_bool implementation *
     *********************************************/

    template <class T, std::size_t N>
    inline avx512_fallback_batch_bool<T, N>::avx512_fallback_batch_bool()
    {
    }

    template <class T, std::size_t N>
    inline avx512_fallback_batch_bool<T, N>::avx512_fallback_batch_bool(bool b)
        : m_value(_mm512_set1_epi64(-(int64_t)b))
    {
    }

    template <class T, std::size_t N>
    template <class... Args, class>
    inline avx512_fallback_batch_bool<T, N>::avx512_fallback_batch_bool(Args... args)
        : m_value(avx512_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{},
                  static_cast<T>(-static_cast<bool>(args))...))
    {
    }

    template <class T, std::size_t N>
    inline avx512_fallback_batch_bool<T, N>::avx512_fallback_batch_bool(const __m512i& rhs)
        : m_value(rhs)
    {
    }

    template <class T, std::size_t N>
    inline avx512_fallback_batch_bool<T, N>::operator __m512i() const
    {
        return m_value;
    }

    template <class T, std::size_t N>
    inline avx512_fallback_batch_bool<T, N>& avx512_fallback_batch_bool<T, N>::operator=(const __m512i& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T, std::size_t N>
    inline bool_proxy<T> avx512_fallback_batch_bool<T, N>::operator[](std::size_t idx)
    {
        return bool_proxy<T>(m_array[idx & (N - 1)]);
    }

    template <class T, std::size_t N>
    inline bool avx512_fallback_batch_bool<T, N>::operator[](std::size_t idx) const
    {
        return static_cast<bool>(m_array[idx & (N - 1)]);
    }

    template <class T, std::size_t N>
    template <class... Args>
    inline batch_bool<T, N>& avx512_fallback_batch_bool<T, N>::load_values(Args... b)
    {
        m_value = avx512_detail::int_init(std::integral_constant<std::size_t, sizeof(T)>{},
                                          static_cast<T>(-static_cast<bool>(b))...);
        return (*this)();
    }
    
    namespace detail
    {
        template <class T, std::size_t N>
        struct avx512_fallback_batch_bool_kernel
        {
            using batch_type = batch_bool<T, N>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_and_si512(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_or_si512(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_xor_si512(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm512_xor_si512(rhs, _mm512_set1_epi64(-1)); // xor with all one
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_andnot_si512(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return ~(lhs ^ rhs);
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return lhs ^ rhs;
            }

            static bool all(const batch_type& rhs)
            {
                XSIMD_SPLIT_AVX512(rhs);
                bool res_hi = _mm256_testc_si256(rhs_high, batch_bool<int32_t, 8>(true)) != 0;
                bool res_lo = _mm256_testc_si256(rhs_low, batch_bool<int32_t, 8>(true)) != 0;
                return res_hi && res_lo;
            }

            static bool any(const batch_type& rhs)
            {
                XSIMD_SPLIT_AVX512(rhs);
                bool res_hi = !_mm256_testz_si256(rhs_high, rhs_high);
                bool res_lo = !_mm256_testz_si256(rhs_low, rhs_low);
                return res_hi || res_lo;
            }
        };
    }
}

#endif
