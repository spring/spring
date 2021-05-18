/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_INT64_HPP
#define XSIMD_NEON_INT64_HPP

#include "xsimd_base.hpp"
#include "xsimd_neon_int_base.hpp"

namespace xsimd
{

    /*********************
     * batch<int64_t, 2> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int64_t, 2>>
    {
        using value_type = int64_t;
        static constexpr std::size_t size = 2;
        using batch_bool_type = batch_bool<int64_t, 2>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
        using storage_type = int64x2_t;
    };

    template <>
    class batch<int64_t, 2> : public simd_batch<batch<int64_t, 2>>
    {
    public:

        using self_type = batch<int64_t, 2>;
        using base_type = simd_batch<self_type>;
        using storage_type = typename base_type::storage_type;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(int64_t src);

        template <class... Args, class Enable = detail::is_array_initializer_t<int64_t, 2, Args...>>
        batch(Args... args);
        explicit batch(const int64_t* src);

        batch(const int64_t* src, aligned_mode);
        batch(const int64_t* src, unaligned_mode);

        batch(const storage_type& rhs);
        batch& operator=(const storage_type& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);

        operator storage_type() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(int64_t, 2)
        XSIMD_DECLARE_LOAD_STORE_LONG(int64_t, 2)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, int32_t rhs);
    batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, int32_t rhs);
    batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs);
    batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs);

    /***********************************
    * batch<int64_t, 2> implementation *
    ************************************/

    inline batch<int64_t, 2>::batch()
    {
    }

    inline batch<int64_t, 2>::batch(int64_t src)
        : base_type(vdupq_n_s64(src))
    {
    }

    template <class... Args, class>
    inline batch<int64_t, 2>::batch(Args... args)
        : base_type(storage_type{static_cast<int64_t>(args)...})
    {
    }

    inline batch<int64_t, 2>::batch(const int64_t* src)
        : base_type(vld1q_s64(src))
    {
    }

    inline batch<int64_t, 2>::batch(const int64_t* src, aligned_mode)
        : batch(src)
    {
    }

    inline batch<int64_t, 2>::batch(const int64_t* src, unaligned_mode)
        : batch(src)
    {
    }

    inline batch<int64_t, 2>::batch(const storage_type& rhs)
        : base_type(rhs)
    {
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::operator=(const storage_type& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    namespace detail
    {
        inline int64x2_t init_from_bool(uint64x2_t a)
        {
            return vandq_s64(reinterpret_cast<int64x2_t>(a), vdupq_n_s64(1));
        }
    }

    inline batch<int64_t, 2>::batch(const batch_bool_type& rhs)
        : base_type(detail::init_from_bool(rhs))
    {
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = detail::init_from_bool(rhs);
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, bool, XSIMD_DEFAULT_ALIGNMENT)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, int8_t, XSIMD_DEFAULT_ALIGNMENT)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, uint8_t, XSIMD_DEFAULT_ALIGNMENT)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, int16_t, XSIMD_DEFAULT_ALIGNMENT)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, uint16_t, XSIMD_DEFAULT_ALIGNMENT)

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_aligned(const int32_t* src)
    {
        int32x2_t tmp = vld1_s32(src);
        this->m_value = vmovl_s32(tmp);
        return *this;
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_unaligned(const int32_t* src)
    {
        return load_aligned(src);
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_aligned(const uint32_t* src)
    {
        uint32x2_t tmp = vld1_u32(src);
        this->m_value = vreinterpretq_s64_u64(vmovl_u32(tmp));
        return *this;
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_unaligned(const uint32_t* src)
    {
        return load_aligned(src);
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_aligned(const int64_t* src)
    {
        this->m_value = vld1q_s64(src);
        return *this;
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_unaligned(const int64_t* src)
    {
        return load_aligned(src);
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_aligned(const uint64_t* src)
    {
        this->m_value = vreinterpretq_s64_u64(vld1q_u64(src));
        return *this;
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_unaligned(const uint64_t* src)
    {
        return load_aligned(src);
    }

    XSIMD_DEFINE_LOAD_STORE_LONG(int64_t, 2, 16)

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_aligned(const float* src)
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        this->m_value = vcvtq_s64_f64(vcvt_f64_f32(vld1_f32(src)));
    #else
        this->m_value = int64x2_t{static_cast<int64_t>(src[0]),
                            static_cast<int64_t>(src[1])};
    #endif
        return *this;
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_unaligned(const float* src)
    {
        return load_aligned(src);
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_aligned(const double* src)
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        this->m_value = vcvtq_s64_f64(vld1q_f64(src));
    #else
        this->m_value = int64x2_t{static_cast<int64_t>(src[0]),
                            static_cast<int64_t>(src[1])};
    #endif
        return *this;
    }

    inline batch<int64_t, 2>& batch<int64_t, 2>::load_unaligned(const double* src)
    {
        return load_aligned(src);
    }

    inline void batch<int64_t, 2>::store_aligned(int32_t* dst) const
    {
        int32x2_t tmp = vmovn_s64(this->m_value);
        vst1_s32((int32_t*)dst, tmp);
    }

    inline void batch<int64_t, 2>::store_unaligned(int32_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<int64_t, 2>::store_aligned(uint32_t* dst) const
    {
        uint32x2_t tmp = vmovn_u64(vreinterpretq_u64_s64(this->m_value));
        vst1_u32((uint32_t*)dst, tmp);
    }

    inline void batch<int64_t, 2>::store_unaligned(uint32_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<int64_t, 2>::store_aligned(int64_t* dst) const
    {
        vst1q_s64(dst, this->m_value);
    }

    inline void batch<int64_t, 2>::store_unaligned(int64_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<int64_t, 2>::store_aligned(uint64_t* dst) const
    {
        vst1q_u64(dst, vreinterpretq_u64_s64(this->m_value));
    }

    inline void batch<int64_t, 2>::store_unaligned(uint64_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<int64_t, 2>::store_aligned(float* dst) const
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1_f32(dst, vcvt_f32_f64(vcvtq_f64_s64(this->m_value)));
    #else
        dst[0] = static_cast<float>(this->m_value[0]);
        dst[1] = static_cast<float>(this->m_value[1]);
    #endif
    }

    inline void batch<int64_t, 2>::store_unaligned(float* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<int64_t, 2>::store_aligned(double* dst) const
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1q_f64(dst, vcvtq_f64_s64(this->m_value));
    #else
        dst[0] = static_cast<double>(this->m_value[0]);
        dst[1] = static_cast<double>(this->m_value[1]);
    #endif
    }

    inline void batch<int64_t, 2>::store_unaligned(double* dst) const
    {
        store_aligned(dst);
    }

    inline batch<int64_t, 2>::operator storage_type() const
    {
        return this->m_value;
    }

    namespace detail
    {
        template <>
        struct batch_kernel<int64_t, 2>
            : neon_int_kernel_base<batch<int64_t, 2>>
        {
            using batch_type = batch<int64_t, 2>;
            using value_type = int64_t;
            using batch_bool_type = batch_bool<int64_t, 2>;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vnegq_s64(rhs);
#else
                return batch<int64_t, 2>(-rhs[0], -rhs[1]);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return vaddq_s64(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return vsubq_s64(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return vqaddq_s64(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return vqsubq_s64(lhs, rhs);
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return { lhs[0] * rhs[0], lhs[1] * rhs[1] };
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION && defined(XSIMD_FAST_INTEGER_DIVISION)
                return vcvtq_s64_f64(vcvtq_f64_s64(lhs) / vcvtq_f64_s64(rhs));
#else
                return{ lhs[0] / rhs[0], lhs[1] / rhs[1] };
#endif
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                return{ lhs[0] % rhs[0], lhs[1] % rhs[1] };
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vceqq_s64(lhs, rhs);
#else
                return batch_bool<int64_t, 2>(lhs[0] == rhs[0], lhs[1] == rhs[1]);
#endif
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return !(lhs == rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vcltq_s64(lhs, rhs);
#else
                return batch_bool<int64_t, 2>(lhs[0] < rhs[0], lhs[1] < rhs[1]);
#endif
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vcleq_s64(lhs, rhs);
#else
                return batch_bool<int64_t, 2>(lhs[0] <= rhs[0], lhs[1] <= rhs[1]);
#endif
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vandq_s64(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vorrq_s64(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return veorq_s64(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vreinterpretq_s64_s32(vmvnq_s32(vreinterpretq_s32_s64(rhs)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vbicq_s64(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return { lhs[0] < rhs[0] ? lhs[0] : rhs[0],
                         lhs[1] < rhs[1] ? lhs[1] : rhs[1] };
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return { lhs[0] > rhs[0] ? lhs[0] : rhs[0],
                         lhs[1] > rhs[1] ? lhs[1] : rhs[1] };
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vabsq_s64(rhs);
#else
                return batch<int64_t, 2>(std::abs(rhs[0]), std::abs(rhs[1]));
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vaddvq_s64(rhs);
#else
                return rhs[0] + rhs[1];
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return vbslq_s64(cond, a, b);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip1q_s64(lhs, rhs);
#else
                return vcombine_s64(vget_low_s64(lhs), vget_low_s64(rhs));
#endif
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip2q_s64(lhs, rhs);
#else
                return vcombine_s64(vget_high_s64(lhs), vget_high_s64(rhs));
#endif
            }
        };
    }



    /*inline batch<int64_t, 2> haddp(const batch<int64_t, 2>* row)
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        return vpaddq_s64(row[0], row[1]);
    #else
        return batch<int64_t, 2>(row[0][0] + row[0][1], row[1][0] + row[1][1]);
    #endif
    }*/

    namespace detail
    {
        inline batch<int64_t, 2> shift_left(const batch<int64_t, 2>& lhs, int32_t n)
        {
            switch(n)
            {
                case 0: return lhs;
                XSIMD_REPEAT_64(vshlq_n_s64);
                default: break;
            }
            return batch<int64_t, 2>(int64_t(0));
        }

        inline batch<int64_t, 2> shift_right(const batch<int64_t, 2>& lhs, int32_t n)
        {
            switch(n)
            {
                case 0: return lhs;
                XSIMD_REPEAT_64(vshrq_n_s64);
                default: break;
            }
            return batch<int64_t, 2>(int64_t(0));
        }
    }

    inline batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, int32_t rhs)
    {
        return detail::shift_left(lhs, rhs);
    }

    inline batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, int32_t rhs)
    {
        return detail::shift_right(lhs, rhs);
    }

    inline batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs)
    {
        return vshlq_s64(lhs, rhs);
    }

    inline batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs)
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        return vshlq_s64(lhs, vnegq_s64(rhs));
#else
        return batch<int64_t, 2>(lhs[0] >> rhs[0], lhs[1] >> rhs[1]);
#endif
    }
}

#endif
