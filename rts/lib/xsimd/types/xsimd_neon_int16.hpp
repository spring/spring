/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_INT16_HPP
#define XSIMD_NEON_INT16_HPP

#include <utility>

#include "xsimd_base.hpp"
#include "xsimd_neon_bool.hpp"
#include "xsimd_neon_int_base.hpp"
#include "xsimd_neon_utils.hpp"

namespace xsimd
{
    /*********************
     * batch<int16_t, 8> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int16_t, 8>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 8;
        using batch_bool_type = batch_bool<int16_t, 8>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
        using storage_type = int16x8_t;
    };

    template <>
    class batch<int16_t, 8> : public simd_batch<batch<int16_t, 8>>
    {
    public:

        using base_type = simd_batch<batch<int16_t, 8>>;
        using storage_type = typename base_type::storage_type;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(int16_t d);

        template <class... Args, class Enable = detail::is_array_initializer_t<int16_t, 8, Args...>>
        batch(Args... args);
        explicit batch(const int16_t* src);

        batch(const int16_t* src, aligned_mode);
        batch(const int16_t* src, unaligned_mode);

        explicit batch(const char* src);
        batch(const char* src, aligned_mode);
        batch(const char* src, unaligned_mode);

        batch(const storage_type& rhs);
        batch& operator=(const storage_type& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);

        operator storage_type() const;

        batch& load_aligned(const int16_t* src);
        batch& load_unaligned(const int16_t* src);

        batch& load_aligned(const uint16_t* src);
        batch& load_unaligned(const uint16_t* src);

        void store_aligned(int16_t* dst) const;
        void store_unaligned(int16_t* dst) const;

        void store_aligned(uint16_t* dst) const;
        void store_unaligned(uint16_t* dst) const;

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT16(int16_t, 8)
        XSIMD_DECLARE_LOAD_STORE_LONG(int16_t, 8)
    };

    batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, int32_t rhs);
    batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, int32_t rhs);
    batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs);
    batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs);

    /************************************
     * batch<int16_t, 8> implementation *
     ************************************/

    inline batch<int16_t, 8>::batch()
    {
    }

    inline batch<int16_t, 8>::batch(int16_t d)
        : base_type(vdupq_n_s16(d))
    {
    }

    template <class... Args, class>
    inline batch<int16_t, 8>::batch(Args... args)
        : base_type(storage_type{static_cast<int16_t>(args)...})
    {
    }

    inline batch<int16_t, 8>::batch(const int16_t* d)
        : base_type(vld1q_s16(d))
    {
    }

    inline batch<int16_t, 8>::batch(const int16_t* d, aligned_mode)
        : batch(d)
    {
    }

    inline batch<int16_t, 8>::batch(const int16_t* d, unaligned_mode)
        : batch(d)
    {
    }

    inline batch<int16_t, 8>::batch(const char* d)
        : batch(reinterpret_cast<const int16_t*>(d))
    {
    }

    inline batch<int16_t, 8>::batch(const char* d, aligned_mode)
        : batch(reinterpret_cast<const int16_t*>(d))
    {
    }

    inline batch<int16_t, 8>::batch(const char* d, unaligned_mode)
        : batch(reinterpret_cast<const int16_t*>(d))
    {
    }

    inline batch<int16_t, 8>::batch(const storage_type& rhs)
        : base_type(rhs)
    {
    }

    inline batch<int16_t, 8>& batch<int16_t, 8>::operator=(const storage_type& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    namespace detail
    {
        inline int16x8_t init_from_bool(uint16x8_t a)
        {
            return vandq_s16(reinterpret_cast<int16x8_t>(a), vdupq_n_s16(1));
        }
    }

    inline batch<int16_t, 8>::batch(const batch_bool_type& rhs)
        : base_type(detail::init_from_bool(rhs))
    {
    }

    inline batch<int16_t, 8>& batch<int16_t, 8>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = detail::init_from_bool(rhs);
        return *this;
    }

    inline batch<int16_t, 8>& batch<int16_t, 8>::load_aligned(const int16_t* src)
    {
        this->m_value = vld1q_s16(src);
        return *this;
    }

    inline batch<int16_t, 8>& batch<int16_t, 8>::load_unaligned(const int16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<int16_t, 8>& batch<int16_t, 8>::load_aligned(const uint16_t* src)
    {
        this->m_value = vreinterpretq_s16_u16(vld1q_u16(src));
        return *this;
    }

    inline batch<int16_t, 8>& batch<int16_t, 8>::load_unaligned(const uint16_t* src)
    {
        return load_aligned(src);
    }

    inline void batch<int16_t, 8>::store_aligned(int16_t* dst) const
    {
        vst1q_s16(dst, this->m_value);
    }

    inline void batch<int16_t, 8>::store_unaligned(int16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<int16_t, 8>::store_aligned(uint16_t* dst) const
    {
        vst1q_u16(dst, vreinterpretq_u16_s16(this->m_value));
    }

    inline void batch<int16_t, 8>::store_unaligned(uint16_t* dst) const
    {
        store_aligned(dst);
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(int16_t, 8, 8)
    XSIMD_DEFINE_LOAD_STORE_LONG(int16_t, 8, 8)

    inline batch<int16_t, 8>::operator int16x8_t() const
    {
        return this->m_value;
    }

    namespace detail
    {

        template <>
        struct batch_kernel<int16_t, 8>
            : neon_int_kernel_base<batch<int16_t, 8>>
        {
            using batch_type = batch<int16_t, 8>;
            using value_type = int16_t;
            using batch_bool_type = batch_bool<int16_t, 8>;

            static batch_type neg(const batch_type& rhs)
            {
                return vnegq_s16(rhs);
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return vaddq_s16(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return vsubq_s16(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return vqaddq_s16(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return vqsubq_s16(lhs, rhs);
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return vmulq_s16(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return neon_detail::unroll_op<8, int16x8_t, int16_t>([&lhs, &rhs] (std::size_t idx) {
                    return lhs[idx] / rhs[idx];
                });
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                return neon_detail::unroll_op<8, int16x8_t, int16_t>([&lhs, &rhs] (std::size_t idx) {
                    return lhs[idx] % rhs[idx];
                });
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_s16(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return !(lhs == rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return vcltq_s16(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return vcleq_s16(lhs, rhs);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vandq_s16(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vorrq_s16(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_s16(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vmvnq_s16(rhs);
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vbicq_s16(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return vminq_s16(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return vmaxq_s16(lhs, rhs);
            }

            static batch_type abs(const batch_type& rhs)
            {
                return vabsq_s16(rhs);
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vaddvq_s16(rhs);
#else
                int16x4_t tmp = vpadd_s16(vget_low_s16(rhs), vget_high_s16(rhs));
                value_type res = 0;
                for (std::size_t i = 0; i < 4; ++i)
                {
                    res += tmp[i];
                }
                return res;
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return vbslq_s16(cond, a, b);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip1q_s16(lhs, rhs);
#else
                int16x4x2_t tmp = vzip_s16(vget_low_s16(lhs), vget_low_s16(rhs));
                return vcombine_s16(tmp.val[0], tmp.val[1]);
#endif
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip2q_s16(lhs, rhs);
#else
                int16x4x2_t tmp = vzip_s16(vget_high_s16(lhs), vget_high_s16(rhs));
                return vcombine_s16(tmp.val[0], tmp.val[1]);
#endif
            }
        };
    }

    namespace detail
    {
        inline batch<int16_t, 8> shift_left(const batch<int16_t, 8>& lhs, int32_t n)
        {
            switch(n)
            {
                case 0: return lhs;
                XSIMD_REPEAT_16(vshlq_n_s16);
                default: break;
            }
            return batch<int16_t, 8>(int16_t(0));
        }

        inline batch<int16_t, 8> shift_right(const batch<int16_t, 8>& lhs, int32_t n)
        {
            switch(n)
            {
                case 0: return lhs;
                XSIMD_REPEAT_16(vshrq_n_s16);
                default: break;
            }
            return batch<int16_t, 8>(int16_t(0));
        }
    }

    inline batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, int32_t rhs)
    {
        return detail::shift_left(lhs, rhs);
    }

    inline batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, int32_t rhs)
    {
        return detail::shift_right(lhs, rhs);
    }

    inline batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs)
    {
        return vshlq_s16(lhs, rhs);
    }

    inline batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs)
    {
        return vshlq_s16(lhs, vnegq_s16(rhs));
    }
}

#endif
