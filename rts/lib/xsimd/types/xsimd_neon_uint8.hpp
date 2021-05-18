/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_UINT8_HPP
#define XSIMD_NEON_UINT8_HPP

#include <utility>

#include "xsimd_base.hpp"
#include "xsimd_neon_bool.hpp"
#include "xsimd_neon_int_base.hpp"
#include "xsimd_neon_utils.hpp"

namespace xsimd
{
    /*********************
     * batch<uint8_t, 16> *
     *********************/

    template <>
    struct simd_batch_traits<batch<uint8_t, 16>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<uint8_t, 16>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
        using storage_type = uint8x16_t;
    };

    template <>
    class batch<uint8_t, 16> : public simd_batch<batch<uint8_t, 16>>
    {
    public:

        using base_type = simd_batch<batch<uint8_t, 16>>;
        using storage_type = typename base_type::storage_type;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(uint8_t d);

        template <class... Args, class Enable = detail::is_array_initializer_t<uint8_t, 16, Args...>>
        batch(Args... args);
        explicit batch(const uint8_t* src);

        batch(const uint8_t* src, aligned_mode);
        batch(const uint8_t* src, unaligned_mode);

        batch(const storage_type& rhs);
        batch& operator=(const storage_type& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);

        operator storage_type() const;

        batch& load_aligned(const int8_t* src);
        batch& load_unaligned(const int8_t* src);

        batch& load_aligned(const uint8_t* src);
        batch& load_unaligned(const uint8_t* src);

        void store_aligned(int8_t* dst) const;
        void store_unaligned(int8_t* dst) const;

        void store_aligned(uint8_t* dst) const;
        void store_unaligned(uint8_t* dst) const;

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT8(uint8_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint8_t, 16)
    };

    batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, int32_t rhs);
    batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, int32_t rhs);
    batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs);
    batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs);

    /************************************
     * batch<int8_t, 16> implementation *
     ************************************/

    inline batch<uint8_t, 16>::batch()
    {
    }

    inline batch<uint8_t, 16>::batch(uint8_t d)
        : base_type(vdupq_n_u8(d))
    {
    }

    template <class... Args, class>
    inline batch<uint8_t, 16>::batch(Args... args)
        : base_type(storage_type{static_cast<uint8_t>(args)...})
    {
    }

    inline batch<uint8_t, 16>::batch(const uint8_t* d)
        : base_type(vld1q_u8(d))
    {
    }

    inline batch<uint8_t, 16>::batch(const uint8_t* d, aligned_mode)
        : batch(d)
    {
    }

    inline batch<uint8_t, 16>::batch(const uint8_t* d, unaligned_mode)
        : batch(d)
    {
    }

    inline batch<uint8_t, 16>::batch(const storage_type& rhs)
        : base_type(rhs)
    {
    }

    inline batch<uint8_t, 16>& batch<uint8_t, 16>::operator=(const storage_type& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<uint8_t, 16>::batch(const batch_bool_type& rhs)
        : base_type(vandq_u8(rhs, batch(1)))
    {
    }

    inline batch<uint8_t, 16>& batch<uint8_t, 16>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = vandq_u8(rhs, batch(1));
        return *this;
    }

    inline batch<uint8_t, 16>& batch<uint8_t, 16>::load_aligned(const int8_t* src)
    {
        this->m_value = vreinterpretq_u8_s8(vld1q_s8(src));
        return *this;
    }

    inline batch<uint8_t, 16>& batch<uint8_t, 16>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }


    inline batch<uint8_t, 16>& batch<uint8_t, 16>::load_aligned(const uint8_t* src)
    {
        this->m_value = vld1q_u8(src);
        return *this;
    }

    inline batch<uint8_t, 16>& batch<uint8_t, 16>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline void batch<uint8_t, 16>::store_aligned(int8_t* dst) const
    {
        vst1q_s8(dst, vreinterpretq_s8_u8(this->m_value));
    }

    inline void batch<uint8_t, 16>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<uint8_t, 16>::store_aligned(uint8_t* dst) const
    {
        vst1q_u8(dst, this->m_value);
    }

    inline void batch<uint8_t, 16>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(uint8_t, 16, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint8_t, 16, 16)

    inline batch<uint8_t, 16>::operator uint8x16_t() const
    {
        return this->m_value;
    }

    namespace detail
    {

        template <>
        struct batch_kernel<uint8_t, 16>
            : neon_int_kernel_base<batch<uint8_t, 16>>
        {
            using batch_type = batch<uint8_t, 16>;
            using value_type = uint8_t;
            using batch_bool_type = batch_bool<uint8_t, 16>;

            static batch_type neg(const batch_type& rhs)
            {
                return vreinterpretq_u8_s8(vnegq_s8(vreinterpretq_s8_u8(rhs)));
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return vaddq_u8(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return vsubq_u8(lhs, rhs);
            }

            static batch_type sadd(const batch_type &lhs, const batch_type &rhs)
            {
                return vqaddq_u8(lhs, rhs);
            }

            static batch_type ssub(const batch_type &lhs, const batch_type &rhs)
            {
                return vqsubq_u8(lhs, rhs);
            }
            
            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return vmulq_u8(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return neon_detail::unroll_op<16, uint8x16_t, uint8_t>([&lhs, &rhs] (std::size_t idx) {
                    return lhs[idx] / rhs[idx];
                });
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                return neon_detail::unroll_op<16, uint8x16_t, uint8_t>([&lhs, &rhs] (std::size_t idx) {
                    return lhs[idx] % rhs[idx];
                });
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_u8(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return !(lhs == rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return vcltq_u8(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return vcleq_u8(lhs, rhs);
            }

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

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return vminq_u8(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return vmaxq_u8(lhs, rhs);
            }

            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            // Not implemented yet
            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vaddvq_u8(rhs);
#else
                uint8x8_t tmp = vpadd_u8(vget_low_u8(rhs), vget_high_u8(rhs));
                value_type res = 0;
                for (std::size_t i = 0; i < 8; ++i)
                {
                    res += tmp[i];
                }
                return res;
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return vbslq_u8(cond, a, b);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip1q_u8(lhs, rhs);
#else
                uint8x8x2_t tmp = vzip_u8(vget_low_u8(lhs), vget_low_u8(rhs));
                return vcombine_u8(tmp.val[0], tmp.val[1]);
#endif
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip2q_u8(lhs, rhs);
#else
                uint8x8x2_t tmp = vzip_u8(vget_high_u8(lhs), vget_high_u8(rhs));
                return vcombine_u8(tmp.val[0], tmp.val[1]);
#endif
            }
        };
    }

    namespace detail
    {
        inline batch<uint8_t, 16> shift_left(const batch<uint8_t, 16>& lhs, int32_t n)
        {
            switch(n)
            {
                case 0: return lhs;
                XSIMD_REPEAT_8(vshlq_n_u8);
                default: break;
            }
            return batch<uint8_t, 16>(uint8_t(0));
        }

        inline batch<uint8_t, 16> shift_right(const batch<uint8_t, 16>& lhs, int32_t n)
        {
            switch(n)
            {
                case 0: return lhs;
                XSIMD_REPEAT_8(vshrq_n_u8);
                default: break;
            }
            return batch<uint8_t, 16>(uint8_t(0));
        }
    }

    inline batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, int32_t rhs)
    {
        return detail::shift_left(lhs, rhs);
    }

    inline batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, int32_t rhs)
    {
        return detail::shift_right(lhs, rhs);
    }

    inline batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs)
    {
        return vshlq_u8(lhs, rhs);
    }

    inline batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs)
    {
        return vshlq_u8(lhs, vnegq_s8(rhs));
    }
}

#endif
