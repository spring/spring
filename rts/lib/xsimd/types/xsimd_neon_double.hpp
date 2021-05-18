/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_DOUBLE_HPP
#define XSIMD_NEON_DOUBLE_HPP

#include "xsimd_base.hpp"

namespace xsimd
{
    template <>
    struct simd_batch_traits<batch<double, 2>>
    {
        using value_type = double;
        static constexpr std::size_t size = 2;
        using batch_bool_type = batch_bool<double, 2>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
        using storage_type = float64x2_t;
    };

    template <>
    class batch<double, 2> : public simd_batch<batch<double, 2>>
    {
    public:

        using self_type = batch<double, 2>;
        using base_type = simd_batch<self_type>;
        using storage_type = typename base_type::storage_type;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(double d);
        batch(double d0, double d1);
        explicit batch(const double* src);

        batch(const double* src, aligned_mode);
        batch(const double* src, unaligned_mode);

        batch(const storage_type& rhs);
        batch& operator=(const storage_type& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);
        
        operator storage_type() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(double, 2)
        XSIMD_DECLARE_LOAD_STORE_LONG(double, 2)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /**
     * Implementation of batch
     */

    inline batch<double, 2>::batch()
    {
    }

    inline batch<double, 2>::batch(double d)
        : base_type(vdupq_n_f64(d))
    {
    }

    inline batch<double, 2>::batch(double d1, double d2)
        : base_type(storage_type{ d1, d2 })
    {
    }

    inline batch<double, 2>::batch(const double* d)
        : base_type(vld1q_f64(d))
    {
    }

    inline batch<double, 2>::batch(const double* d, aligned_mode)
        : batch(d)
    {
    }

    inline batch<double, 2>::batch(const double* d, unaligned_mode)
        : batch(d)
    {
    }

    inline batch<double, 2>::batch(const storage_type& rhs)
        : base_type(rhs)
    {
    }

    inline batch<double, 2>& batch<double, 2>::operator=(const storage_type& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<double, 2>::batch(const batch_bool_type& rhs)
        : base_type(vreinterpretq_f64_u64(vandq_u64(rhs,
                                                    vreinterpretq_u64_f64(batch(1.)))))
    {
    }

    inline batch<double, 2>& batch<double, 2>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = vreinterpretq_f64_u64(vandq_u64(rhs,
                                                    vreinterpretq_u64_f64(batch(1.))));
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const int8_t* src)
    {
        int8x8_t tmp = vld1_s8((const int8_t*)src);
        int16x8_t tmp2 = vmovl_s8(tmp);
        int16x4_t tmp3 = vget_low_s16(tmp2);
        int32x4_t tmp4 = vmovl_s16(tmp3);
        float32x4_t tmp5 = vcvtq_f32_s32(tmp4);
        float32x2_t tmp6 = vget_low_f32(tmp5);
        this->m_value = vcvt_f64_f32(tmp6);
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const uint8_t* src)
    {
        uint8x8_t tmp = vld1_u8((const uint8_t*)src);
        uint16x8_t tmp2 = vmovl_u8(tmp);
        uint16x4_t tmp3 = vget_low_u16(tmp2);
        uint32x4_t tmp4 = vmovl_u16(tmp3);
        float32x4_t tmp5 = vcvtq_f32_u32(tmp4);
        float32x2_t tmp6 = vget_low_f32(tmp5);
        this->m_value = vcvt_f64_f32(tmp6);
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(double, 2, bool, XSIMD_DEFAULT_ALIGNMENT)

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const int16_t* src)
    {
        int16x4_t tmp1 = vld1_s16(src);
        int32x4_t tmp2 = vmovl_s16(tmp1);
        float32x4_t tmp3 = vcvtq_f32_s32(tmp2);
        float32x2_t tmp4 = vget_low_f32(tmp3);
        this->m_value = vcvt_f64_f32(tmp4);
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const int16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const uint16_t* src)
    {
        uint16x4_t tmp1 = vld1_u16(src);
        uint32x4_t tmp2 = vmovl_u16(tmp1);
        float32x4_t tmp3 = vcvtq_f32_u32(tmp2);
        float32x2_t tmp4 = vget_low_f32(tmp3);
        this->m_value = vcvt_f64_f32(tmp4);
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const uint16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const int32_t* d)
    {
        this->m_value = vcvt_f64_f32(vcvt_f32_s32(vld1_s32(d)));
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const int32_t* d)
    {
        return load_aligned(d);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const uint32_t* d)
    {
        this->m_value = vcvt_f64_f32(vcvt_f32_u32(vld1_u32(d)));
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const uint32_t* d)
    {
        return load_aligned(d);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const int64_t* d)
    {
        this->m_value = vcvtq_f64_s64(vld1q_s64(d));
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const int64_t* d)
    {
        return load_aligned(d);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const uint64_t* d)
    {
        this->m_value = vcvtq_f64_u64(vld1q_u64(d));
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const uint64_t* d)
    {
        return load_aligned(d);
    }

    XSIMD_DEFINE_LOAD_STORE_LONG(double, 2, 16)

    inline batch<double, 2>& batch<double, 2>::load_aligned(const float* d)
    {
        this->m_value = vcvt_f64_f32(vld1_f32(d));
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const float* d)
    {
        return load_aligned(d);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const double* d)
    {
        this->m_value = vld1q_f64(d);
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const double* d)
    {
        return load_aligned(d);
    }

    inline void batch<double, 2>::store_aligned(int8_t* dst) const
    {
        float32x2_t tmp = vcvt_f32_f64(this->m_value);
        int32x2_t tmp2 = vcvtn_s32_f32(tmp);
        int32x4_t tmp3 = vcombine_s32(tmp2, vdup_n_s32(0));
        int16x4_t tmp4 = vmovn_s32(tmp3);
        int16x8_t tmp5 = vcombine_s16(tmp4, vdup_n_s16(0));
        int8x8_t tmp6 = vmovn_s16(tmp5);
        vst1_s8((int8_t*)dst, tmp6);
    }

    inline void batch<double, 2>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(uint8_t* dst) const
    {
        float32x2_t tmp = vcvt_f32_f64(this->m_value);
        uint32x2_t tmp2 = vcvtn_u32_f32(tmp);
        uint32x4_t tmp3 = vcombine_u32(tmp2, vdup_n_u32(0));
        uint16x4_t tmp4 = vmovn_u32(tmp3);
        uint16x8_t tmp5 = vcombine_u16(tmp4, vdup_n_u16(0));
        uint8x8_t tmp6 = vmovn_u16(tmp5);
        vst1_u8((uint8_t*)dst, tmp6);
    }

    inline void batch<double, 2>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(int16_t* dst) const
    {
        float32x2_t tmp = vcvt_f32_f64(this->m_value);
        int32x2_t tmp2 = vcvtn_s32_f32(tmp);
        int32x4_t tmp3 = vcombine_s32(tmp2, vdup_n_s32(0));
        int16x4_t tmp4 = vmovn_s32(tmp3);
        vst1_s16((int16_t*)dst, tmp4);
    }

    inline void batch<double, 2>::store_unaligned(int16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(uint16_t* dst) const
    {
        float32x2_t tmp = vcvt_f32_f64(this->m_value);
        uint32x2_t tmp2 = vcvtn_u32_f32(tmp);
        uint32x4_t tmp3 = vcombine_u32(tmp2, vdup_n_u32(0));
        uint16x4_t tmp4 = vmovn_u32(tmp3);
        vst1_u16((uint16_t*)dst, tmp4);
    }

    inline void batch<double, 2>::store_unaligned(uint16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(int32_t* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1_s32(dst, vcvt_s32_f32(vcvt_f32_f64(m_value)));
#else
        dst[0] = static_cast<int32_t>(this->m_value[0]);
        dst[1] = static_cast<int32_t>(this->m_value[1]);
#endif
    }

    inline void batch<double, 2>::store_unaligned(int32_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(uint32_t* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1_u32(dst, vcvt_u32_f32(vcvt_f32_f64(m_value)));
#else
        dst[0] = static_cast<uint32_t>(this->m_value[0]);
        dst[1] = static_cast<uint32_t>(this->m_value[1]);
#endif
    }

    inline void batch<double, 2>::store_unaligned(uint32_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(int64_t* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1q_s64(dst, vcvtq_s64_f64(m_value));
#else
        dst[0] = static_cast<int64_t>(this->m_value[0]);
        dst[1] = static_cast<int64_t>(this->m_value[1]);
#endif
    }

    inline void batch<double, 2>::store_unaligned(int64_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(uint64_t* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1q_u64(dst, vcvtq_u64_f64(m_value));
#else
        dst[0] = static_cast<uint64_t>(this->m_value[0]);
        dst[1] = static_cast<uint64_t>(this->m_value[1]);
#endif
    }

    inline void batch<double, 2>::store_unaligned(uint64_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(float* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        vst1_f32(dst, vcvt_f32_f64(m_value));
#else
        dst[0] = static_cast<float>(this->m_value[0]);
        dst[1] = static_cast<float>(this->m_value[1]);
#endif
    }

    inline void batch<double, 2>::store_unaligned(float* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(double* dst) const
    {
        vst1q_f64(dst, m_value);
    }

    inline void batch<double, 2>::store_unaligned(double* dst) const
    {
        store_aligned(dst);
    }

    inline batch<double, 2>::operator float64x2_t() const
    {
        return this->m_value;
    }

    namespace detail
    {
        template <>
        struct batch_kernel<double, 2>
        {
            using batch_type = batch<double, 2>;
            using value_type = double;
            using batch_bool_type = batch_bool<double, 2>;

            static batch_type neg(const batch_type& rhs)
            {
                return vnegq_f64(rhs);
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return vaddq_f64(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return vsubq_f64(lhs, rhs);
            }

			      static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return add(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return sub(lhs, rhs);
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return vmulq_f64(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vdivq_f64(lhs, rhs);
#else
                // from stackoverflow & https://projectne10.github.io/Ne10/doc/NE10__divc_8neon_8c_source.html
                // get an initial estimate of 1/b.
                float64x2_t reciprocal = vrecpeq_f64(rhs);

                // use a couple Newton-Raphson steps to refine the estimate.  Depending on your
                // application's accuracy requirements, you may be able to get away with only
                // one refinement (instead of the two used here).  Be sure to test!
                reciprocal = vmulq_f64(vrecpsq_f64(rhs, reciprocal), reciprocal);
                reciprocal = vmulq_f64(vrecpsq_f64(rhs, reciprocal), reciprocal);

                // and finally, compute a / b = a * (1 / b)
                return vmulq_f64(lhs, reciprocal);
#endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_f64(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return !(lhs == rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return vcltq_f64(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return vcleq_f64(lhs, rhs);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f64_u64(vandq_u64(vreinterpretq_u64_f64(lhs),
                                                       vreinterpretq_u64_f64(rhs)));
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f64_u64(vorrq_u64(vreinterpretq_u64_f64(lhs),
                                                       vreinterpretq_u64_f64(rhs)));
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(lhs),
                                                       vreinterpretq_u64_f64(rhs)));
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vreinterpretq_f64_u32(vmvnq_u32(vreinterpretq_u32_f64(rhs)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f64_u64(vbicq_u64(vreinterpretq_u64_f64(lhs), vreinterpretq_u64_f64(rhs)));
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return vminq_f64(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return vmaxq_f64(lhs, rhs);
            }

            static batch_type fmin(const batch_type& lhs, const batch_type& rhs)
            {
                return min(lhs, rhs);
            }

            static batch_type fmax(const batch_type& lhs, const batch_type& rhs)
            {
                return max(lhs, rhs);
            }

            static batch_type abs(const batch_type& rhs)
            {
                return vabsq_f64(rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vsqrtq_f64(rhs);
#else
                float64x2_t sqrt_reciprocal = vrsqrteq_f64(rhs);
                // one iter
                // sqrt_reciprocal = sqrt_reciprocal * vrsqrtsq_f64(lhs * sqrt_reciprocal, sqrt_reciprocal);
                return rhs * sqrt_reciprocal * vrsqrtsq_f64(rhs * sqrt_reciprocal, sqrt_reciprocal);
#endif
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return vfmaq_f64(z, x, y);
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return vfmaq_f64(-z, x, y);
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return fma(-x, y, z);
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return fms(-x, y, z);
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vaddvq_f64(rhs);
#else
                float64x2_t tmp = vpaddq_f64(rhs, rhs);
                return vgetq_lane_f64(tmp, 0);
#endif
            }

            static batch_type haddp(const simd_batch<batch_type>* row)
            {
                return vpaddq_f64(row[0](), row[1]());
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return vbslq_f64(cond, a, b);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip1q_f64(lhs, rhs);
#else
                return vcombine_f64(vget_low_f64(lhs), vget_low_f64(rhs));
#endif
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip2q_f64(lhs, rhs);
#else
                return vcombine_f64(vget_high_f64(lhs), vget_high_f64(rhs));
#endif
            }

            static batch_bool_type isnan(const batch_type& x)
            {
                return !(x == x);
            }
        };
    }
}

#endif
