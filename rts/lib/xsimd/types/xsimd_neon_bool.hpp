/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_BOOL_HPP
#define XSIMD_NEON_BOOL_HPP

#include "xsimd_base.hpp"

#define EXPAND(...) __VA_ARGS__

#define CASE(op, i)                   \
    case i: return op(lhs, i);

#define XSIMD_REPEAT_8_0(op, addx)    \
    CASE(EXPAND(op), 1 + addx);       \
    CASE(EXPAND(op), 2 + addx);       \
    CASE(EXPAND(op), 3 + addx);       \
    CASE(EXPAND(op), 4 + addx);       \
    CASE(EXPAND(op), 5 + addx);       \
    CASE(EXPAND(op), 6 + addx);       \
    CASE(EXPAND(op), 7 + addx);

#define XSIMD_REPEAT_8_N(op, addx)    \
    CASE(EXPAND(op), 0 + addx);       \
    XSIMD_REPEAT_8_0(op, addx);

#define XSIMD_REPEAT_8(op)            \
    XSIMD_REPEAT_8_0(op, 0);

#define XSIMD_REPEAT_16_0(op, addx)   \
    XSIMD_REPEAT_8_0(op, 0 + addx);   \
    XSIMD_REPEAT_8_N(op, 8 + addx);

#define XSIMD_REPEAT_16_N(op, addx)   \
    XSIMD_REPEAT_8_N(op, 0 + addx);   \
    XSIMD_REPEAT_8_N(op, 8 + addx);

#define XSIMD_REPEAT_16(op)           \
    XSIMD_REPEAT_16_0(op, 0);

#define XSIMD_REPEAT_32_0(op, addx)   \
    XSIMD_REPEAT_16_0(op, 0 + addx);  \
    XSIMD_REPEAT_16_N(op, 16 + addx);

#define XSIMD_REPEAT_32_N(op, addx)   \
    XSIMD_REPEAT_16_N(op, 0 + addx);  \
    XSIMD_REPEAT_16_N(op, 16 + addx);

#define XSIMD_REPEAT_32(op)           \
    XSIMD_REPEAT_32_0(op, 0);

#define XSIMD_REPEAT_64(op)           \
    XSIMD_REPEAT_32_0(op, 0);         \
    XSIMD_REPEAT_32_N(op, 32);

namespace xsimd
{
    /********************
     * batch_bool<T, 8> *
     ********************/

    template <class T>
    struct simd_batch_traits<batch_bool<T, 8>>
    {
        using value_type = T;
        static constexpr std::size_t size = 8;
        using batch_type = batch<T, 8>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
    };

    template <class T>
    class batch_bool<T, 8> : public simd_batch_bool<batch_bool<T, 8>>
    {
    public:

        using simd_type = uint16x8_t;

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1, bool b2, bool b3,
                   bool b4, bool b5, bool b6, bool b7);
        batch_bool(const simd_type& rhs);
        template <class V>
        batch_bool(const batch_bool<V, 8>& rhs);

        batch_bool& operator=(const simd_type& rhs);

        operator simd_type() const;

        bool_proxy<uint16_t> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

    private:

        batch_bool<T, 8>& load_values(bool b0, bool b1, bool b2, bool b3,
                                      bool b4, bool b5, bool b6, bool b7);
        union
        {
            simd_type m_value;
            uint16_t m_array[8];
        };

        friend class simd_batch_bool<batch_bool<T, 8>>;
    };

    /********************
     * batch_bool<T, 4> *
     ********************/

    template <class T>
    struct simd_batch_traits<batch_bool<T, 4>>
    {
        using value_type = T;
        static constexpr std::size_t size = 4;
        using batch_type = batch<T, 4>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
    };

    template <class T>
    class batch_bool<T, 4> : public simd_batch_bool<batch_bool<T, 4>>
    {
    public:

        using simd_type = uint32x4_t;

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1, bool b2, bool b3);
        batch_bool(const simd_type& rhs);
        template <class V>
        batch_bool(const batch_bool<V, 4>& rhs);

        batch_bool& operator=(const simd_type& rhs);

        operator simd_type() const;

        bool_proxy<uint32_t> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

    private:

        batch_bool<T, 4>& load_values(bool b0, bool b1, bool b2, bool b3);

        union
        {
            simd_type m_value;
            uint32_t m_array[4];
        };

        friend class simd_batch_bool<batch_bool<T, 4>>;
    };

    /********************
     * batch_bool<T, 2> *
     ********************/

    template <class T>
    struct simd_batch_traits<batch_bool<T, 2>>
    {
        using value_type = T;
        static constexpr std::size_t size = 2;
        using batch_type = batch<T, 2>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
    };

    template <class T>
    class batch_bool<T, 2> : public simd_batch_bool<batch_bool<T, 2>>
    {
    public:

        using simd_type = uint64x2_t;

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1);
        batch_bool(const simd_type& rhs);
        template <class V>
        batch_bool(const batch_bool<V, 2>& rhs);

        batch_bool& operator=(const simd_type& rhs);

        operator simd_type() const;

        bool_proxy<uint64_t> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

    private:

        batch_bool<T, 2>& load_values(bool b0, bool b1);

        union
        {
            simd_type m_value;
            uint64_t m_array[2];
        };

        friend class simd_batch_bool<batch_bool<T, 2>>;
    };

    /*********************
     * batch_bool<T, 16> *
     *********************/

    template <class T>
    struct simd_batch_traits<batch_bool<T, 16>>
    {
        using value_type = T;
        static constexpr std::size_t size = 16;
        using batch_type = batch<T, 16>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
    };

    template <class T>
    class batch_bool<T, 16> : public simd_batch_bool<batch_bool<T, 16>>
    {
    public:

        using simd_type = uint8x16_t;

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool d1,  bool d2,  bool d3,  bool d4,
                   bool d5,  bool d6,  bool d7,  bool d8,
                   bool d9, bool d10, bool d11, bool d12,
                   bool d13, bool d14, bool d15, bool d16);
        batch_bool(const simd_type& rhs);
        template <class V>
        batch_bool(const batch_bool<V, 16>& rhs);

        batch_bool& operator=(const simd_type& rhs);

        operator simd_type() const;

        bool_proxy<uint8_t> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

    private:

        batch_bool<T, 16>& load_values(bool d1,  bool d2,  bool d3,  bool d4,
                                       bool d5,  bool d6,  bool d7,  bool d8,
                                       bool d9, bool d10, bool d11, bool d12,
                                       bool d13, bool d14, bool d15, bool d16);

        union
        {
            simd_type m_value;
            uint8_t m_array[16];
        };

        friend class simd_batch_bool<batch_bool<T, 16>>;
    };

    /***********************************
     * batch_bool<T, 8> implementation *
     ***********************************/

    template <class T>
    inline batch_bool<T, 8>::batch_bool()
    {
    }

    template <class T>
    inline batch_bool<T, 8>::batch_bool(bool b)
        : m_value(vdupq_n_u16(
            static_cast<uint16_t>(-(int)b))
        )
    {
    }

    template <class T>
    inline batch_bool<T, 8>::batch_bool(bool b0, bool b1, bool b2, bool b3,
                                        bool b4, bool b5, bool b6, bool b7)
        : m_value{
            static_cast<uint16_t>(-int(b0)),
            static_cast<uint16_t>(-int(b1)),
            static_cast<uint16_t>(-int(b2)),
            static_cast<uint16_t>(-int(b3)),
            static_cast<uint16_t>(-int(b4)),
            static_cast<uint16_t>(-int(b5)),
            static_cast<uint16_t>(-int(b6)),
            static_cast<uint16_t>(-int(b7))}
    {
    }

    template <class T>
    inline batch_bool<T, 8>::batch_bool(const simd_type& rhs)
        : m_value(rhs)
    {
    }

    template <class T>
    template <class V>
    inline batch_bool<T, 8>::batch_bool(const batch_bool<V, 8>& rhs)
        : m_value(static_cast<simd_type>(rhs))
    {
    }

    template <class T>
    inline batch_bool<T, 8>& batch_bool<T, 8>::operator=(const simd_type& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T>
    inline batch_bool<T, 8>::operator uint16x8_t() const
    {
        return m_value;
    }

    template <class T>
    inline bool_proxy<uint16_t> batch_bool<T, 8>::operator[](std::size_t index)
    {
        return bool_proxy<uint16_t>(m_array[index & 7]);
    }

    template <class T>
    inline bool batch_bool<T, 8>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 7]);
    }

    template <class T>
    inline batch_bool<T, 8>& batch_bool<T, 8>::load_values(bool b0, bool b1, bool b2, bool b3,
                                                           bool b4, bool b5, bool b6, bool b7)
    {
        m_value[0] = static_cast<uint16_t>(-int(b0));
        m_value[1] = static_cast<uint16_t>(-int(b1));
        m_value[2] = static_cast<uint16_t>(-int(b2));
        m_value[3] = static_cast<uint16_t>(-int(b3));
        m_value[4] = static_cast<uint16_t>(-int(b4));
        m_value[5] = static_cast<uint16_t>(-int(b5));
        m_value[6] = static_cast<uint16_t>(-int(b6));
        m_value[7] = static_cast<uint16_t>(-int(b7));
        return *this;
    }

    namespace detail
    {
        template <class T>
        struct batch_bool_kernel<T, 8>
        {
            using batch_type = batch_bool<T, 8>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vandq_u16(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vorrq_u16(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u16(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vmvnq_u16(rhs);
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vbicq_u16(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_u16(lhs, rhs);
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u16(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vminvq_u16(rhs) != 0;
#else
                uint16x4_t tmp = vand_u16(vget_low_u16(rhs), vget_high_u16(rhs));
                tmp = vpmin_u16(tmp, tmp);
                tmp = vpmin_u16(tmp, tmp);
                return vget_lane_u16(tmp, 0) != 0;
#endif
            }

            static bool any(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vmaxvq_u16(rhs) != 0;
#else
                uint16x4_t tmp = vorr_u16(vget_low_u16(rhs), vget_high_u16(rhs));
                tmp = vpmax_u16(tmp, tmp);
                tmp = vpmax_u16(tmp, tmp);
                return vget_lane_u16(tmp, 0);
#endif
            }
        };
    }

    /***********************************
     * batch_bool<T, 4> implementation *
     ***********************************/

    template <class T>
    inline batch_bool<T, 4>::batch_bool()
    {
    }

    template <class T>
    inline batch_bool<T, 4>::batch_bool(bool b)
        : m_value(vdupq_n_u32(
            static_cast<uint32_t>(-(int)b))
        )
    {
    }

    template <class T>
    inline batch_bool<T, 4>::batch_bool(bool b0, bool b1, bool b2, bool b3)
        : m_value{
            static_cast<uint32_t>(-int(b0)),
            static_cast<uint32_t>(-int(b1)),
            static_cast<uint32_t>(-int(b2)),
            static_cast<uint32_t>(-int(b3))}
    {
    }

    template <class T>
    inline batch_bool<T, 4>::batch_bool(const simd_type& rhs)
        : m_value(rhs)
    {
    }

    template <class T>
    template <class V>
    inline batch_bool<T, 4>::batch_bool(const batch_bool<V, 4>& rhs)
        : m_value(static_cast<simd_type>(rhs))
    {
    }

    template <class T>
    inline batch_bool<T, 4>& batch_bool<T, 4>::operator=(const simd_type& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T>
    inline batch_bool<T, 4>::operator uint32x4_t() const
    {
        return m_value;
    }

    template <class T>
    inline bool_proxy<uint32_t> batch_bool<T, 4>::operator[](std::size_t index)
    {
        return bool_proxy<uint32_t>(m_array[index & 3]);
    }

    template <class T>
    inline bool batch_bool<T, 4>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 3]);
    }

    template <class T>
    inline batch_bool<T, 4>& batch_bool<T, 4>::load_values(bool b0, bool b1, bool b2, bool b3)
    {
        m_value[0] = static_cast<uint32_t>(-int(b0));
        m_value[1] = static_cast<uint32_t>(-int(b1));
        m_value[2] = static_cast<uint32_t>(-int(b2));
        m_value[3] = static_cast<uint32_t>(-int(b3));
        return *this;
    }

    namespace detail
    {
        template <class T>
        struct batch_bool_kernel<T, 4>
        {
            using batch_type = batch_bool<T, 4>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vandq_u32(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vorrq_u32(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u32(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vmvnq_u32(rhs);
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vbicq_u32(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_u32(lhs, rhs);
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u32(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vminvq_u32(rhs) != 0;
#else
                uint32x2_t tmp = vand_u32(vget_low_u32(rhs), vget_high_u32(rhs));
                return vget_lane_u32(vpmin_u32(tmp, tmp), 0) != 0;
#endif
            }

            static bool any(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vmaxvq_u32(rhs) != 0;
#else
                uint32x2_t tmp = vorr_u32(vget_low_u32(rhs), vget_high_u32(rhs));
                return vget_lane_u32(vpmax_u32(tmp, tmp), 0);
#endif
            }
        };
    }

    /************************************
     * batch_bool<T, 16> implementation *
     ************************************/

    template <class T>
    inline batch_bool<T, 16>::batch_bool()
    {
    }

    template <class T>
    inline batch_bool<T, 16>::batch_bool(bool b)
        : m_value(vdupq_n_u8(
            static_cast<uint8_t>(-(int8_t)b))
        )
    {
    }

    template <class T>
    inline batch_bool<T, 16>::batch_bool(bool b1,  bool b2,  bool b3,  bool b4,
                                         bool b5,  bool b6,  bool b7,  bool b8,
                                         bool b9, bool b10, bool b11, bool b12,
                                         bool b13, bool b14, bool b15, bool b16)
        : m_value{
            static_cast<uint8_t>(-int(b1)),
            static_cast<uint8_t>(-int(b2)),
            static_cast<uint8_t>(-int(b3)),
            static_cast<uint8_t>(-int(b4)),
            static_cast<uint8_t>(-int(b5)),
            static_cast<uint8_t>(-int(b6)),
            static_cast<uint8_t>(-int(b7)),
            static_cast<uint8_t>(-int(b8)),
            static_cast<uint8_t>(-int(b9)),
            static_cast<uint8_t>(-int(b10)),
            static_cast<uint8_t>(-int(b11)),
            static_cast<uint8_t>(-int(b12)),
            static_cast<uint8_t>(-int(b13)),
            static_cast<uint8_t>(-int(b14)),
            static_cast<uint8_t>(-int(b15)),
            static_cast<uint8_t>(-int(b16))}
    {
    }

    template <class T>
    inline batch_bool<T, 16>::batch_bool(const simd_type& rhs)
        : m_value(rhs)
    {
    }

    template <class T>
    template <class V>
    inline batch_bool<T, 16>::batch_bool(const batch_bool<V, 16>& rhs)
        : m_value(static_cast<simd_type>(rhs))
    {
    }

    template <class T>
    inline batch_bool<T, 16>& batch_bool<T, 16>::operator=(const simd_type& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T>
    inline batch_bool<T, 16>::operator uint8x16_t() const
    {
        return m_value;
    }

    template <class T>
    inline bool_proxy<uint8_t> batch_bool<T, 16>::operator[](std::size_t index)
    {
        return bool_proxy<uint8_t>(m_array[index & 15]);
    }

    template <class T>
    inline bool batch_bool<T, 16>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 15]);
    }

    template <class T>
    inline batch_bool<T, 16>& batch_bool<T, 16>::load_values(bool b1,  bool b2,  bool b3,  bool b4,
                                                             bool b5,  bool b6,  bool b7,  bool b8,
                                                             bool b9,  bool b10, bool b11, bool b12,
                                                             bool b13, bool b14, bool b15, bool b16)
    {
        m_value[0] = static_cast<uint8_t>(-int(b1));
        m_value[1] = static_cast<uint8_t>(-int(b2));
        m_value[2] = static_cast<uint8_t>(-int(b3));
        m_value[3] = static_cast<uint8_t>(-int(b4));
        m_value[4] = static_cast<uint8_t>(-int(b5));
        m_value[5] = static_cast<uint8_t>(-int(b6));
        m_value[6] = static_cast<uint8_t>(-int(b7));
        m_value[7] = static_cast<uint8_t>(-int(b8));
        m_value[8] = static_cast<uint8_t>(-int(b9));
        m_value[9] = static_cast<uint8_t>(-int(b10));
        m_value[10] = static_cast<uint8_t>(-int(b11));
        m_value[11] = static_cast<uint8_t>(-int(b12));
        m_value[12] = static_cast<uint8_t>(-int(b13));
        m_value[13] = static_cast<uint8_t>(-int(b14));
        m_value[14] = static_cast<uint8_t>(-int(b15));
        m_value[15] = static_cast<uint8_t>(-int(b16));
        return *this;
    }

    namespace detail
    {
        template <class T>
        struct batch_bool_kernel<T, 16>
        {
            using batch_type = batch_bool<T, 16>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vandq_u8(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vorrq_u8(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u8(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vmvnq_u8(rhs);
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vbicq_u8(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_u8(lhs, rhs);
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u8(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vminvq_u8(rhs) != 0;
#else
                uint8x8_t tmp = vand_u8(vget_low_u8(rhs), vget_high_u8(rhs));
                tmp = vpmin_u8(tmp, tmp);
                tmp = vpmin_u8(tmp, tmp);
                tmp = vpmin_u8(tmp, tmp);
                return vget_lane_u8(tmp, 0) != 0;
#endif
            }

            static bool any(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vmaxvq_u8(rhs) != 0;
#else
                uint8x8_t tmp = vorr_u8(vget_low_u8(rhs), vget_high_u8(rhs));
                tmp = vpmax_u8(tmp, tmp);
                tmp = vpmax_u8(tmp, tmp);
                tmp = vpmax_u8(tmp, tmp);
                return vget_lane_u8(tmp, 0);
#endif
            }
        };
    }

    /***********************************
     * batch_bool<T, 2> implementation *
     ***********************************/

    template <class T>
    inline batch_bool<T, 2>::batch_bool()
    {
    }

    template <class T>
    inline batch_bool<T, 2>::batch_bool(bool b)
        : m_value(vdupq_n_u64(static_cast<uint64_t>(-(int)b)))
    {
    }

    template <class T>
    inline batch_bool<T, 2>::batch_bool(bool b0, bool b1)
        : m_value{
            static_cast<uint64_t>(-int(b0)),
            static_cast<uint64_t>(-int(b1))}
    {
    }

    template <class T>
    inline batch_bool<T, 2>::batch_bool(const simd_type& rhs)
        : m_value(rhs)
    {
    }

    template <class T>
    template <class V>
    inline batch_bool<T, 2>::batch_bool(const batch_bool<V, 2>& rhs)
        : m_value(static_cast<simd_type>(rhs))
    {
    }

    template <class T>
    inline batch_bool<T, 2>& batch_bool<T, 2>::operator=(const simd_type& rhs)
    {
        m_value = rhs;
        return *this;
    }

    template <class T>
    inline batch_bool<T, 2>::operator uint64x2_t() const
    {
        return m_value;
    }

    template <class T>
    inline bool_proxy<uint64_t> batch_bool<T, 2>::operator[](std::size_t index)
    {
        return bool_proxy<uint64_t>(m_array[index & 1]);
    }

    template <class T>
    inline bool batch_bool<T, 2>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 1]);
    }

    template <class T>
    inline batch_bool<T, 2>& batch_bool<T, 2>::load_values(bool b0, bool b1)
    {
        m_value[0] = static_cast<uint64_t>(-int(b0));
        m_value[1] = static_cast<uint64_t>(-int(b1));
        return *this;
    }

    namespace detail
    {
        template <class T>
        struct batch_bool_kernel<T, 2>
        {
            using batch_type = batch_bool<T, 2>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vandq_u64(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vorrq_u64(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u64(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(rhs)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                // According to Eigen
                return vbicq_u64(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vceqq_u64(lhs, rhs);
#else
                return vreinterpretq_u64_u32(vceqq_u32(vreinterpretq_u32_u64(lhs), vreinterpretq_u32_u64(rhs)));
#endif
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_u64(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
                uint64x1_t tmp = vand_u64(vget_low_u64(rhs), vget_high_u64(rhs));
                return vget_lane_u64(tmp, 0) != 0;
            }

            static bool any(const batch_type& rhs)
            {
                uint64x1_t tmp = vorr_u64(vget_low_u64(rhs), vget_high_u64(rhs));
                return bool(vget_lane_u64(tmp, 0));
            }
        };
    }
}
#endif
