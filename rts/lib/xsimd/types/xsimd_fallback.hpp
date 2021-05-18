/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_FALLBACK_HPP
#define XSIMD_FALLBACK_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "xsimd_scalar.hpp"

#include "xsimd_base.hpp"
#include "xsimd_complex_base.hpp"
#include "xsimd_utils.hpp"

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

namespace xsimd
{

    /***********************************************************
     * Generic fallback implementation of batch and batch_bool *
     *                                                         *
     * Basically, generate a scalar loop and cross fingers:    *
     * maybe the compiler will autovectorize, maybe not.       *
     ***********************************************************/

    /********************
     * batch_bool<T, N> *
     ********************/

    template <typename T, std::size_t N>
    struct simd_batch_traits<batch_bool<T, N>>
    {
        using value_type = T;
        static constexpr std::size_t size = N;
        using batch_type = batch<T, N>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
    };

    template <typename T, std::size_t N>
    class batch_bool : public simd_batch_bool<batch_bool<T, N>>
    {
    public:

        batch_bool();
        explicit batch_bool(bool b);

        // Constructor from N boolean parameters
        template <
            typename... Args,
            typename Enable = detail::is_array_initializer_t<bool, N, Args...>
        >
        batch_bool(Args... exactly_N_bools);

        batch_bool(const std::array<bool, N>& rhs);
        batch_bool& operator=(const std::array<bool, N>& rhs);

        operator std::array<bool, N>() const;

        const bool& operator[](std::size_t index) const;
        bool& operator[](std::size_t index);

        const std::array<bool, N>& get_value() const;

    private:

        template <class... Args>
        batch_bool<T, N>& load_values(Args... args);
        
        std::array<bool, N> m_value;

        friend class simd_batch_bool<batch_bool<T, N>>;
    };

    /***************
     * batch<T, N> *
     ***************/

    template <typename T, std::size_t N>
    struct simd_batch_traits<batch<T, N>>
    {
        using value_type = T;
        static constexpr std::size_t size = N;
        using batch_bool_type = batch_bool<T, N>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
        using storage_type = std::array<T, N>;
    };

    template <typename T, std::size_t N>
    class batch : public simd_batch<batch<T, N>>
    {
    public:

        using self_type = batch<T, N>;
        using base_type = simd_batch<self_type>;
        using storage_type = typename base_type::storage_type;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(T f);

        // Constructor from N scalar parameters
        template <
            typename... Args,
            typename Enable = typename detail::is_array_initializer<T, N, Args...>::type
        >
        batch(Args... exactly_N_scalars);

        // Constructor from value_type of batch_bool
        batch(const std::array<bool, N>& src);

        explicit batch(const T* src);
        batch(const T* src, aligned_mode);
        batch(const T* src, unaligned_mode);
        batch(const std::array<T, N>& rhs);
        batch(const batch_bool_type& rhs);
        batch& operator=(const std::array<T, N>& rhs);
        batch& operator=(const std::array<bool, N>& rhs);
        batch& operator=(const batch_bool_type&);

        operator std::array<T, N>() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(T, N)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        T& operator[](std::size_t index);
        const T& operator[](std::size_t index) const;

    private:

        template<typename U>
        batch& load_unaligned_impl(const U* src);
        template<typename U>
        void store_unaligned_impl(U* src) const;
    };

    template <typename T, std::size_t N>
    batch<T, N> operator<<(const batch<T, N>& lhs, int32_t rhs);
    template <typename T, std::size_t N>
    batch<T, N> operator>>(const batch<T, N>& lhs, int32_t rhs);

    /**********************************
     * batch_bool<std::complex<T>, N> *
     **********************************/

    template <class T, std::size_t N>
    struct simd_batch_traits<batch_bool<std::complex<T>, N>>
        : complex_batch_bool_traits<std::complex<T>, T, N, XSIMD_DEFAULT_ALIGNMENT>
    {
    };

    template <class T, std::size_t N>
    class batch_bool<std::complex<T>, N>
        : public simd_complex_batch_bool<batch_bool<std::complex<T>, N>>
    {
    public:

        using self_type = batch_bool<std::complex<T>, N>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<T, N>;

        batch_bool() = default;
        using base_type::base_type;

        // Constructor from N boolean parameters
        template <
            typename... Args,
            typename Enable = detail::is_array_initializer_t<bool, N, Args...>
        >
        batch_bool(Args... exactly_N_bools)
            : base_type(real_batch{ exactly_N_bools... })
        {
        }
    };

    /*****************************
     * batch<std::complex<T>, N> *
     *****************************/

    template <class T, std::size_t N>
    struct simd_batch_traits<batch<std::complex<T>, N>>
        : complex_batch_traits<std::complex<T>, T, N, XSIMD_DEFAULT_ALIGNMENT>
    {
    };

    template <class T, std::size_t N>
    class batch<std::complex<T>, N>
        : public simd_complex_batch<batch<std::complex<T>, N>>
    {
    public:

        using self_type = batch<std::complex<T>, N>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<T>;
        using real_batch = batch<T, N>;
        using real_value_type = T;

        batch() = default;
        using base_type::base_type;

        // Constructor from N scalar parameters
        template <
            typename... Args,
            typename Enable = typename detail::is_array_initializer<T, N, Args...>::type
        >
        batch(Args... exactly_N_scalars)
            : base_type(real_batch{ exactly_N_scalars.real()... },
                        real_batch{ exactly_N_scalars.imag()... })
        {
        }

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        template <class U>
        typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
        load_aligned(const U* src);
        template <class U>
        typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
        load_unaligned(const U* src);

        template <class U>
        typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
        load_aligned(const U* src);
        template <class U>
        typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
        load_unaligned(const U* src);

        template <class U>
        void store_aligned(U* dst) const;
        template <class U>
        void store_unaligned(U* dst) const;
    };

#ifdef XSIMD_ENABLE_XTL_COMPLEX

    /********************************************
     * batch_bool<xtl::xcomplex<T, T, i3ec>, N> *
     ********************************************/

    template <class T, std::size_t N, bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<T, T, i3ec>, N>>
        : complex_batch_bool_traits<xtl::xcomplex<T, T, i3ec>, T, N, XSIMD_DEFAULT_ALIGNMENT>
    {
    };

    template<class T, std::size_t N, bool i3ec>
    class batch_bool<xtl::xcomplex<T, T, i3ec>, N>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<T, T, i3ec>, N>>
    {
    public:

        using self_type = batch_bool<xtl::xcomplex<T, T, i3ec>, N>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<T, N>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        // Constructor from N boolean parameters
        template <
            typename... Args,
            typename Enable = detail::is_array_initializer_t<bool, N, Args...>
        >
        batch_bool(Args... exactly_N_bools)
            : base_type(real_batch{ exactly_N_bools... })
        {
        }
    };

    /***************************************
     * batch<xtl::xcomplex<T, T, i3ec>, N> *
     ***************************************/

    template <class T, std::size_t N, bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<T, T, i3ec>, N>>
        : complex_batch_traits<xtl::xcomplex<T, T, i3ec>, T, N, XSIMD_DEFAULT_ALIGNMENT>
    {
    };

    template <class T, std::size_t N, bool i3ec>
    class batch<xtl::xcomplex<T, T, i3ec>, N>
        : public simd_complex_batch<batch<xtl::xcomplex<T, T, i3ec>, N>>
    {
    public:

        using self_type = batch<xtl::xcomplex<T, T, i3ec>, N>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = xtl::xcomplex<T, T, i3ec>;
        using real_batch = batch<T, N>;
        using real_value_type = T;

        batch() = default;
        using base_type::base_type;

        // Constructor from N scalar parameters
        template <
            typename... Args,
            typename Enable = typename detail::is_array_initializer<T, N, Args...>::type
        >
        batch(Args... exactly_N_scalars)
            : base_type(real_batch{ exactly_N_scalars.real()... },
                        real_batch{ exactly_N_scalars.imag()... })
        {
        }

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        template <class U>
        typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
        load_aligned(const U* src);
        template <class U>
        typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
        load_unaligned(const U* src);

        template <class U>
        typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
        load_aligned(const U* src);
        template <class U>
        typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
        load_unaligned(const U* src);

        template <class U>
        void store_aligned(U* dst) const;
        template <class U>
        void store_unaligned(U* dst) const;
    };
#endif

    /************************
     * conversion functions *
     ************************/

    template <std::size_t N>
    batch<int32_t, N> to_int(const batch<float, N>& x);
    template <std::size_t N>
    batch<int64_t, N> to_int(const batch<double, N>& x);

    template <std::size_t N>
    batch<float, N> to_float(const batch<int32_t, N>& x);
    template <std::size_t N>
    batch<double, N> to_float(const batch<int64_t, N>& x);

    /**************************
     * boolean cast functions *
     **************************/

    template <std::size_t N>
    batch_bool<int32_t, N> bool_cast(const batch_bool<float, N>& x);
    template <std::size_t N>
    batch_bool<int64_t, N> bool_cast(const batch_bool<double, N>& x);
    template <std::size_t N>
    batch_bool<float, N> bool_cast(const batch_bool<int32_t, N>& x);
    template <std::size_t N>
    batch_bool<double, N> bool_cast(const batch_bool<int64_t, N>& x);

    /**************************
     * Boilerplate generators *
     **************************/

// These macros all asume that T and N are in scope and have the meaning used in
// the definitions of batch and batch_bool.
#define XSIMD_FALLBACK_MAPPING_LOOP(RESULT_TYPE, EXPRESSION)  \
    RESULT_TYPE<T, N> result;  \
    for(std::size_t i = 0; i < N; ++i) {  \
        result[i] = (EXPRESSION);  \
    }  \
    return result;

#define XSIMD_FALLBACK_UNARY_OP(RESULT_TYPE, OPERATOR, X)  \
    XSIMD_FALLBACK_MAPPING_LOOP(RESULT_TYPE, (OPERATOR X[i]))

#define XSIMD_FALLBACK_BINARY_OP(RESULT_TYPE, OPERATOR, X, Y)  \
    XSIMD_FALLBACK_MAPPING_LOOP(RESULT_TYPE, (X[i] OPERATOR Y[i]))

#define XSIMD_FALLBACK_BATCH_BITWISE_UNARY_OP(OPERATOR, X)  \
    XSIMD_FALLBACK_MAPPING_LOOP(  \
        batch,  \
        detail::from_unsigned_integer<T>(  \
            OPERATOR detail::to_unsigned_integer(X[i])  \
        )  \
    )

#define XSIMD_FALLBACK_BATCH_BITWISE_BINARY_OP(OPERATOR, X, Y)  \
    XSIMD_FALLBACK_MAPPING_LOOP(  \
        batch,  \
        detail::from_unsigned_integer<T>(  \
            detail::to_unsigned_integer(X[i])  \
            OPERATOR  \
            detail::to_unsigned_integer(Y[i])  \
        )  \
    )

#define XSIMD_FALLBACK_BATCH_UNARY_FUNC(FUNCTION, X)  \
    XSIMD_FALLBACK_MAPPING_LOOP(batch, FUNCTION(X[i]))

#define XSIMD_FALLBACK_BATCH_BINARY_FUNC(FUNCTION, X, Y)  \
    XSIMD_FALLBACK_MAPPING_LOOP(batch, FUNCTION(X[i], Y[i]))

#define XSIMD_FALLBACK_BATCH_TERNARY_FUNC(FUNCTION, X, Y, Z)  \
    XSIMD_FALLBACK_MAPPING_LOOP(batch, FUNCTION(X[i], Y[i], Z[i]))

// NOTE: Static casting a vector is static casting every element
#define XSIMD_FALLBACK_BATCH_STATIC_CAST(T_OUT, X)  \
    batch<T_OUT, N> result;  \
    for(std::size_t i = 0; i < N; ++i) {  \
        result[i] = static_cast<T_OUT>(X[i]);  \
    }  \
    return result;

// NOTE: Casting between batch_bools of the same size is actually trivial!
#define XSIMD_FALLBACK_BOOL_CAST(T_OUT, X)  \
    return batch_bool<T_OUT, N>(static_cast<std::array<bool, N>>(X));

    /***********************************
     * batch_bool<T, N> implementation *
     ***********************************/

    template <typename T, std::size_t N>
    inline batch_bool<T, N>::batch_bool()
    {
    }

    template <typename T, std::size_t N>
    inline batch_bool<T, N>::batch_bool(bool b)
        : m_value(detail::array_from_scalar<bool, N>(b))
    {
    }

    template <typename T, std::size_t N>
    template <typename... Args, typename Enable>
    inline batch_bool<T, N>::batch_bool(Args... exactly_N_bools)
        : m_value{ exactly_N_bools... }
    {
    }

    template <typename T, std::size_t N>
    inline batch_bool<T, N>::batch_bool(const std::array<bool, N>& rhs)
        : m_value(rhs)
    {
    }

    template <typename T, std::size_t N>
    inline batch_bool<T, N>& batch_bool<T, N>::operator=(const std::array<bool, N>& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <typename T, std::size_t N>
    inline batch_bool<T, N>::operator std::array<bool, N>() const
    {
        return m_value;
    }

    template <typename T, std::size_t N>
    inline const bool& batch_bool<T, N>::operator[](std::size_t index) const
    {
        return m_value[index];
    }

    template <typename T, std::size_t N>
    inline bool& batch_bool<T, N>::operator[](std::size_t index)
    {
        return m_value[index];
    }

    template <typename T, std::size_t N>
    inline const std::array<bool, N>& batch_bool<T, N>::get_value() const
    {
        return m_value;
    }

    template <typename T, std::size_t N>
    template <class... Args>
    inline batch_bool<T, N>& batch_bool<T, N>::load_values(Args... args)
    {
        m_value = std::array<bool, N>({args...});
        return *this;
    }

    namespace detail
    {
        template <class T, std::size_t N>
        struct batch_bool_kernel
        {
            using batch_type = batch_bool<T, N>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, &, lhs, rhs)
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, | , lhs, rhs)
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, ^, lhs, rhs)
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                XSIMD_FALLBACK_UNARY_OP(batch_bool, !, rhs)
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_MAPPING_LOOP(batch_bool, (!(lhs[i] & rhs[i])))
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, == , lhs, rhs)
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, != , lhs, rhs)
            }

            static bool all(const batch_type& rhs)
            {
                for (std::size_t i = 0; i < N; ++i)
                {
                    if (!rhs[i]) return false;
                }
                return true;
            }

            static bool any(const batch_type& rhs)
            {
                for (std::size_t i = 0; i < N; ++i)
                {
                    if (rhs[i]) return true;
                }
                return false;
            }
        };
    }

    /**********************************
     * batch<T, N> implementation *
     **********************************/

    template <typename T, std::size_t N>
    inline batch<T, N>::batch()
    {
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(T f)
        : base_type(detail::array_from_scalar<T, N>(f))
    {
    }

    template <typename T, std::size_t N>
    template <typename... Args, typename Enable>
    inline batch<T, N>::batch(Args... exactly_N_scalars)
        : base_type(storage_type{ static_cast<T>(exactly_N_scalars)... })
    {
    }

    namespace detail
    {
        template <bool integral>
        struct all_bits
        {
            template <class T>
            static T get(T)
            {
                return ~T(0);
            }
        };

        template <>
        struct all_bits<false>
        {
            template <class T>
            static T get(T)
            {
                T res(0);
                using int_type = as_unsigned_integer_t<T>;
                int_type value(~int_type(0));
                std::memcpy(&res, &value, sizeof(int_type));
                return res;
            }
        };
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(const std::array<bool, N>& src)
    {
        using all_bits = detail::all_bits<std::is_integral<T>::value>;
        for(std::size_t i = 0; i < N; ++i)
        {
            this->m_value[i] = src[i] ? all_bits::get(T(0)) : T(0);
        }
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(const T* src)
        : batch(src, unaligned_mode())
    {
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(const T* src, aligned_mode)
        : batch(src, unaligned_mode())
    {
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(const T* src, unaligned_mode)
        : base_type(detail::array_from_pointer<T, N>(src))
    {
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(const std::array<T, N>& rhs)
        : base_type(rhs)
    {
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::batch(const batch_bool_type& rhs)
    {
        std::transform(rhs.get_value().cbegin(), rhs.get_value().cend(), this->m_value.begin(),
                       [](bool b) -> T { return b ? T(1) : T(0); });
    }

    template <typename T, std::size_t N>
    inline batch<T, N>& batch<T, N>::operator=(const std::array<T, N>& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    template <typename T, std::size_t N>
    inline batch<T, N>& batch<T, N>::operator=(const std::array<bool, N>& rhs)
    {
        using all_bits = detail::all_bits<std::is_integral<T>::value>;
        std::transform(rhs.cbegin(), rhs.cend(), this->m_value.begin(),
                       [](bool b) -> T { return b ? all_bits::get(T(0)) : T(0); });
        return *this;
    }

    template <typename T, std::size_t N>
    inline batch<T, N>& batch<T, N>::operator=(const batch_bool_type& rhs)
    {
        std::transform(rhs.get_value().cbegin(), rhs.get_value().cend(), this->m_value.begin(),
                       [](bool b) -> T { return b ? T(1) : T(0); });
        return *this;
    }

    template <typename T, std::size_t N>
    inline batch<T, N>::operator std::array<T, N>() const
    {
        return this->m_value;
    }

#define FALLBACK_DEFINE_LOAD_STORE(TYPE)                             \
    template <typename T, std::size_t N>                             \
    inline batch<T, N>& batch<T, N>::load_aligned(const TYPE* src)   \
    {                                                                \
        return this->load_unaligned_impl(src);                       \
    }                                                                \
    template <typename T, std::size_t N>                             \
    inline batch<T, N>& batch<T, N>::load_unaligned(const TYPE* src) \
    {                                                                \
        return this->load_unaligned_impl(src);                       \
    }                                                                \
    template <typename T, std::size_t N>                             \
    inline void batch<T, N>::store_aligned(TYPE* dst) const          \
    {                                                                \
        this->store_unaligned_impl(dst);                             \
    }                                                                \
    template <typename T, std::size_t N>                             \
    inline void batch<T, N>::store_unaligned(TYPE* dst) const        \
    {                                                                \
        this->store_unaligned_impl(dst);                             \
    }

    FALLBACK_DEFINE_LOAD_STORE(bool)
    FALLBACK_DEFINE_LOAD_STORE(int8_t)
    FALLBACK_DEFINE_LOAD_STORE(uint8_t)
    FALLBACK_DEFINE_LOAD_STORE(int16_t)
    FALLBACK_DEFINE_LOAD_STORE(uint16_t)
    FALLBACK_DEFINE_LOAD_STORE(int32_t)
    FALLBACK_DEFINE_LOAD_STORE(uint32_t)
    FALLBACK_DEFINE_LOAD_STORE(int64_t)
    FALLBACK_DEFINE_LOAD_STORE(uint64_t)
    FALLBACK_DEFINE_LOAD_STORE(float)
    FALLBACK_DEFINE_LOAD_STORE(double)

#undef FALLBACK_DEFINE_LOAD_STORE

    template <typename T, std::size_t N>
    template <typename U>
    inline batch<T, N>& batch<T, N>::load_unaligned_impl(const U* src)
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            this->m_value[i] = static_cast<T>(src[i]);
        }
        return *this;
    }

    template <typename T, std::size_t N>
    template <typename U>
    inline void batch<T, N>::store_unaligned_impl(U* dst) const
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            dst[i] = static_cast<U>(this->m_value[i]);
        }
    }

    template <typename T, std::size_t N>
    inline T& batch<T, N>::operator[](std::size_t index)
    {
        return this->m_value[index % base_type::size];
    }

    template <typename T, std::size_t N>
    inline const T& batch<T, N>::operator[](std::size_t index) const
    {
        return this->m_value[index % base_type::size];
    }

    namespace detail
    {
        template <class T, std::size_t N>
        struct batch_kernel
        {
            using batch_type = batch<T, N>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, N>;

            static batch_type neg(const batch_type& rhs)
            {
                XSIMD_FALLBACK_UNARY_OP(batch, -, rhs)
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch, +, lhs, rhs)
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch, -, lhs, rhs)
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BINARY_FUNC(xsimd::sadd, lhs, rhs)
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BINARY_FUNC(xsimd::ssub, lhs, rhs)
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch, *, lhs, rhs)
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch, /, lhs, rhs)
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch, % , lhs, rhs)
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, == , lhs, rhs)
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, != , lhs, rhs)
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, < , lhs, rhs)
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BINARY_OP(batch_bool, <=, lhs, rhs)
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BITWISE_BINARY_OP(&, lhs, rhs)
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BITWISE_BINARY_OP(|, lhs, rhs)
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BITWISE_BINARY_OP(^ , lhs, rhs)
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BITWISE_UNARY_OP(~, rhs)
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_MAPPING_LOOP(
                    batch,
                    detail::from_unsigned_integer<T>(
                        ~(
                            detail::to_unsigned_integer(lhs[i])
                            &
                            detail::to_unsigned_integer(rhs[i])
                            )
                        )
                )
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BINARY_FUNC(std::min, lhs, rhs)
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BINARY_FUNC(std::max, lhs, rhs)
            }

            static batch_type fmin(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BINARY_FUNC(std::fmin, lhs, rhs)
            }

            static batch_type fmax(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_BINARY_FUNC(std::fmax, lhs, rhs)
            }

            static batch_type abs(const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::abs, rhs)
            }

            static batch_type fabs(const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::fabs, rhs)
            }

            static batch_type sqrt(const batch_type& rhs)
            {
                XSIMD_FALLBACK_BATCH_UNARY_FUNC(std::sqrt, rhs)
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                XSIMD_FALLBACK_BATCH_TERNARY_FUNC(std::fma, x, y, z)
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return fma(x, y, -z);
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return fma(-x, y, z);
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return fma(-x, y, -z);
            }

            static value_type hadd(const batch_type& rhs)
            {
                value_type result = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                    result += rhs[i];
                }
                return result;
            }

            static batch_type haddp(const batch_type* row)
            {
                XSIMD_FALLBACK_MAPPING_LOOP(batch, hadd(row[i]))
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                XSIMD_FALLBACK_MAPPING_LOOP(batch, (cond[i] ? a[i] : b[i]))
            }

            template<bool... Values>
            static batch_type select(const batch_bool_constant<value_type, Values...>& cond, const batch_type& a, const batch_type& b)
            {
                XSIMD_FALLBACK_MAPPING_LOOP(batch, (cond[i] ? a[i] : b[i]))
            }

            static batch_bool_type isnan(const batch_type& x)
            {
                XSIMD_FALLBACK_MAPPING_LOOP(batch_bool, std::isnan(x[i]))
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                batch_type b_lo;
                for (std::size_t i = 0, j = 0; i < N/2; ++i, j = j + 2)
                {
                    b_lo[j] = lhs[i];
                    b_lo[j + 1] = rhs[i];
                }
                return b_lo;
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                batch_type b_hi;
                for (std::size_t i = 0, j = 0; i < N/2; ++i, j = j + 2)
                {
                    b_hi[j] = lhs[i + N/2];
                    b_hi[j + 1] = rhs[i+ N/2];
                }
                return b_hi;
            }
        };
    }

    template <typename T, std::size_t N>
    inline batch<T, N> operator<<(const batch<T, N>& lhs, int32_t rhs) {
        XSIMD_FALLBACK_MAPPING_LOOP(batch, (lhs[i] << rhs))
    }

    template <typename T, std::size_t N>
    inline batch<T, N> operator>>(const batch<T, N>& lhs, int32_t rhs) {
        XSIMD_FALLBACK_MAPPING_LOOP(batch, (lhs[i] >> rhs))
    }

    /***********************************************
     * utility functions to avoid code duplication *
     ***********************************************/

    namespace detail
    {
        template <std::size_t N, class B, class U>
        inline std::pair<B, B> load_complex_impl(const U* src)
        {
            using value_type = typename U::value_type;
            using dst_value_type = typename B::value_type;
            const value_type* buf = reinterpret_cast<const value_type*>(src);
            B real, imag;
            for (std::size_t i = 0; i < N; ++i)
            {
                real[i] = static_cast<dst_value_type>(buf[2 * i]);
                imag[i] = static_cast<dst_value_type>(buf[2 * i + 1]);
            }
            return std::make_pair(real, imag);
        }

        template <std::size_t N, class B, class U>
        inline void store_complex_impl(const B& real, const B& imag, U* dst)
        {
            using value_type = typename U::value_type;
            value_type* buf = reinterpret_cast<value_type*>(dst);
            for (std::size_t i = 0; i < N; ++i)
            {
                buf[2 * i] = static_cast<value_type>(real[i]);
                buf[2 * i + 1] = static_cast<value_type>(imag[i]);
            }
        }
    }

    /********************************************
     * batch<std::complex<T, N>> implementation *
     ********************************************/

    template <class T, std::size_t N>
    template <class U>
    inline auto batch<std::complex<T>, N>::load_aligned(const U* src)
        -> typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
    {
        std::tie(this->m_real, this->m_imag) = detail::load_complex_impl<N, real_batch>(src);
        return *this;
    }

    template <class T, std::size_t N>
    template <class U>
    inline auto batch<std::complex<T>, N>::load_unaligned(const U* src)
        -> typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
    {
        return load_aligned(src);
    }

    template <class T, std::size_t N>
    template <class U>
    inline auto batch<std::complex<T>, N>::load_aligned(const U* src)
        -> typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            this->m_real[i] = static_cast<real_value_type>(src[i]);
            this->m_imag[i] = real_value_type(0);
        }
        return *this;
    }

    template <class T, std::size_t N>
    template <class U>
    inline auto batch<std::complex<T>, N>::load_unaligned(const U* src)
        -> typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
    {
        return load_aligned(src);
    }

    template <class T, std::size_t N>
    template <class U>
    inline void batch<std::complex<T>, N>::store_aligned(U* dst) const
    {
        detail::store_complex_impl<N>(this->m_real, this->m_imag, dst);
    }

    template <class T, std::size_t N>
    template <class U>
    inline void batch<std::complex<T>, N>::store_unaligned(U* dst) const
    {
        store_aligned(dst);
    }

    /******************************************************
     * batch<xtl::xcomplex<T, T, i3ec>, N> implementation *
     ******************************************************/

#ifdef XSIMD_ENABLE_XTL_COMPLEX

    template <class T, std::size_t N, bool i3ec>
    template <class U>
    inline auto batch<xtl::xcomplex<T, T, i3ec>, N>::load_aligned(const U* src)
        -> typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
    {
        std::tie(this->m_real, this->m_imag) = detail::load_complex_impl<N, real_batch>(src);
        return *this;
    }

    template <class T, std::size_t N, bool i3ec>
    template <class U>
    inline auto batch<xtl::xcomplex<T, T, i3ec>, N>::load_unaligned(const U* src)
        -> typename std::enable_if<detail::is_complex<U>::value, self_type&>::type
    {
        return load_aligned(src);
    }

    template <class T, std::size_t N, bool i3ec>
    template <class U>
    inline auto batch<xtl::xcomplex<T, T, i3ec>, N>::load_aligned(const U* src)
        -> typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            this->m_real[i] = static_cast<real_value_type>(src[i]);
            this->m_imag[i] = real_value_type(0);
        }
        return *this;
    }

    template <class T, std::size_t N, bool i3ec>
    template <class U>
    inline auto batch<xtl::xcomplex<T, T, i3ec>, N>::load_unaligned(const U* src)
        -> typename std::enable_if<!detail::is_complex<U>::value, self_type&>::type
    {
        return load_aligned(src);
    }

    template <class T, std::size_t N, bool i3ec>
    template <class U>
    inline void batch<xtl::xcomplex<T, T, i3ec>, N>::store_aligned(U* dst) const
    {
        detail::store_complex_impl<N>(this->m_real, this->m_imag, dst);
    }

    template <class T, std::size_t N, bool i3ec>
    template <class U>
    inline void batch<xtl::xcomplex<T, T, i3ec>, N>::store_unaligned(U* dst) const
    {
        store_unaligned(dst);
    }

#endif

    /***************************************
     * conversion functions implementation *
     ***************************************/

    template <std::size_t N>
    inline batch<int32_t, N> to_int(const batch<float, N>& x)
    {
        XSIMD_FALLBACK_BATCH_STATIC_CAST(int32_t, x)
    }

    template <std::size_t N>
    inline batch<int64_t, N> to_int(const batch<double, N>& x)
    {
        XSIMD_FALLBACK_BATCH_STATIC_CAST(int64_t, x)
    }

    template <std::size_t N>
    inline batch<float, N> to_float(const batch<int32_t, N>& x)
    {
        XSIMD_FALLBACK_BATCH_STATIC_CAST(float, x)
    }

    template <std::size_t N>
    inline batch<double, N> to_float(const batch<int64_t, N>& x)
    {
        XSIMD_FALLBACK_BATCH_STATIC_CAST(double, x)
    }

    /**************************
     * boolean cast functions *
     **************************/

    template <std::size_t N>
    inline batch_bool<int32_t, N> bool_cast(const batch_bool<float, N>& x)
    {
        XSIMD_FALLBACK_BOOL_CAST(int32_t, x)
    }

    template <std::size_t N>
    inline batch_bool<int64_t, N> bool_cast(const batch_bool<double, N>& x)
    {
        XSIMD_FALLBACK_BOOL_CAST(int64_t, x)
    }

    template <std::size_t N>
    inline batch_bool<float, N> bool_cast(const batch_bool<int32_t, N>& x)
    {
        XSIMD_FALLBACK_BOOL_CAST(float, x)
    }

    template <std::size_t N>
    inline batch_bool<double, N> bool_cast(const batch_bool<int64_t, N>& x)
    {
        XSIMD_FALLBACK_BOOL_CAST(double, x)
    }

    /*****************************************
     * bitwise cast functions implementation *
     *****************************************/

    template <class T_in, class T_out, std::size_t N_in>
    struct bitwise_cast_impl<batch<T_in, N_in>,
                             batch<T_out, sizeof(T_in)*N_in/sizeof(T_out)>>
    {
    private:
        static_assert(sizeof(T_in)*N_in % sizeof(T_out) == 0,
                      "The input and output batches must have the same size");
        static constexpr size_t N_out = sizeof(T_in)*N_in/sizeof(T_out);

        union Converter {
            std::array<T_in, N_in> in;
            std::array<T_out, N_out> out;
        };

    public:
        static batch<T_out, N_out> run(const batch<T_in, N_in>& x) {
            Converter caster;
            caster.in = static_cast<std::array<T_in, N_in>>(x);
            return batch<T_out, N_out>(caster.out);
        }
    };
}

#endif
