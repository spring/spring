/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_FLOAT_HPP
#define XSIMD_NEON_FLOAT_HPP

#include "xsimd_base.hpp"
#include "xsimd_neon_bool.hpp"

namespace xsimd
{
    /*******************
     * batch<float, 4> *
     *******************/
    
    template <>
    struct simd_batch_traits<batch<float, 4>>
    {
        using value_type = float;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<float, 4>;
        static constexpr std::size_t align = XSIMD_DEFAULT_ALIGNMENT;
        using storage_type = float32x4_t;
    };

    template <>
    class batch<float, 4> : public simd_batch<batch<float, 4>>
    {
    public:

        using self_type = batch<float, 4>;
        using base_type = simd_batch<self_type>;
        using storage_type = typename base_type::storage_type;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(float d);
        batch(float d0, float d1, float d2, float d3);
        explicit batch(const float* src);

        batch(const float* src, aligned_mode);
        batch(const float* src, unaligned_mode);

        batch(const storage_type& rhs);
        batch& operator=(const storage_type& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);

        operator storage_type() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(float, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(float, 4)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /**********************************
     * batch<float, 4> implementation *
     **********************************/

    inline batch<float, 4>::batch()
    {
    }

    inline batch<float, 4>::batch(float d)
        : base_type(vdupq_n_f32(d))
    {
    }

    inline batch<float, 4>::batch(float d1, float d2, float d3, float d4)
        : base_type(storage_type{d1, d2, d3, d4})
    {
    }

    inline batch<float, 4>::batch(const float* d)
        : base_type(vld1q_f32(d))
    {
    }

    inline batch<float, 4>::batch(const float* d, aligned_mode)
        : batch(d)
    {
    }

    inline batch<float, 4>::batch(const float* d, unaligned_mode)
        : batch(d)
    {
    }

    inline batch<float, 4>::batch(const storage_type& rhs)
        : base_type(rhs)
    {
    }

    inline batch<float, 4>& batch<float, 4>::operator=(const storage_type& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<float, 4>::batch(const batch_bool_type& rhs)
        : base_type(vreinterpretq_f32_u32(vandq_u32(rhs, 
                                                    vreinterpretq_u32_f32(batch(1.f)))))
    {
    }

    inline batch<float, 4>& batch<float, 4>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = vreinterpretq_f32_u32(vandq_u32(rhs, vreinterpretq_u32_f32(batch(1.f))));
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int8_t* src)
    {
        int8x8_t tmp = vld1_s8((const int8_t*)src);
        int16x8_t tmp2 = vmovl_s8(tmp);
        int16x4_t tmp3 = vget_low_s16(tmp2);
        int32x4_t tmp4 = vmovl_s16(tmp3);
        this->m_value = vcvtq_f32_s32(tmp4);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }

    XSIMD_DEFINE_LOAD_STORE(float, 4, bool, XSIMD_DEFAULT_ALIGNMENT)

    inline batch<float, 4>& batch<float, 4>::load_aligned(const uint8_t* src)
    {
        uint8x8_t tmp = vld1_u8((const uint8_t*)src);
        uint16x8_t tmp2 = vmovl_u8(tmp);
        uint16x4_t tmp3 = vget_low_u16(tmp2);
        uint32x4_t tmp4 = vmovl_u16(tmp3);
        this->m_value = vcvtq_f32_u32(tmp4);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int16_t* src)
    {
        int16x4_t tmp1 = vld1_s16(src);
        int32x4_t tmp2 = vmovl_s16(tmp1);
        this->m_value = vcvtq_f32_s32(tmp2);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const uint16_t* src)
    {
        uint16x4_t tmp1 = vld1_u16(src);
        uint32x4_t tmp2 = vmovl_u16(tmp1);
        this->m_value = vcvtq_f32_u32(tmp2);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const uint16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int32_t* d)
    {
        this->m_value = vcvtq_f32_s32(vld1q_s32(d));
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int32_t* d)
    {
        return load_aligned(d);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const uint32_t* d)
    {
        this->m_value = vcvtq_f32_u32(vld1q_u32(d));
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const uint32_t* d)
    {
        return load_aligned(d);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int64_t* d)
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        float32x2_t tmp_l = vcvt_f32_f64(vcvtq_f64_s64(vld1q_s64(&d[0])));
        float32x2_t tmp_h = vcvt_f32_f64(vcvtq_f64_s64(vld1q_s64(&d[2])));
        this->m_value = vcombine_f32(tmp_l, tmp_h);
    #else
        this->m_value = float32x4_t{
            static_cast<float>(d[0]),
            static_cast<float>(d[1]),
            static_cast<float>(d[2]),
            static_cast<float>(d[3])
        };
    #endif
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int64_t* d)
    {
        return load_aligned(d);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const uint64_t* d)
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        float32x2_t tmp_l = vcvt_f32_f64(vcvtq_f64_u64(vld1q_u64(&d[0])));
        float32x2_t tmp_h = vcvt_f32_f64(vcvtq_f64_u64(vld1q_u64(&d[2])));
        this->m_value = vcombine_f32(tmp_l, tmp_h);
    #else
        this->m_value = float32x4_t{
            static_cast<float>(d[0]),
            static_cast<float>(d[1]),
            static_cast<float>(d[2]),
            static_cast<float>(d[3])
        };
    #endif
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const uint64_t* d)
    {
        return load_aligned(d);
    }

    XSIMD_DEFINE_LOAD_STORE_LONG(float, 4, 16)

    inline batch<float, 4>& batch<float, 4>::load_aligned(const float* d)
    {
        this->m_value = vld1q_f32(d);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const float* d)
    {
        return load_aligned(d);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const double* d)
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        float32x2_t tmp_l = vcvt_f32_f64(vld1q_f64(&d[0]));
        float32x2_t tmp_h = vcvt_f32_f64(vld1q_f64(&d[2]));
        this->m_value = vcombine_f32(tmp_l, tmp_h);
        return *this;
    #else
        this->m_value = float32x4_t{
            static_cast<float>(d[0]),
            static_cast<float>(d[1]),
            static_cast<float>(d[2]),
            static_cast<float>(d[3])
        };
    #endif
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const double* d)
    {
        return load_aligned(d);
    }

    inline void batch<float, 4>::store_aligned(int8_t* dst) const
    {
        int32x4_t tmp = vcvtq_s32_f32(this->m_value);
        int16x4_t tmp2 = vmovn_s32(tmp);
        int16x8_t tmp3 = vcombine_s16(tmp2, vdup_n_s16(0));
        int8x8_t tmp4 = vmovn_s16(tmp3);
        vst1_s8((int8_t*)dst, tmp4);
    }

    inline void batch<float, 4>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(uint8_t* dst) const
    {
        uint32x4_t tmp = vcvtq_u32_f32(this->m_value);
        uint16x4_t tmp2 = vmovn_u32(tmp);
        uint16x8_t tmp3 = vcombine_u16(tmp2, vdup_n_u16(0));
        uint8x8_t tmp4 = vmovn_u16(tmp3);
        vst1_u8((uint8_t*)dst, tmp4);
    }

    inline void batch<float, 4>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(int16_t* dst) const
    {
        int32x4_t tmp = vcvtq_s32_f32(this->m_value);
        int16x4_t tmp2 = vmovn_s32(tmp);
        vst1_s16((int16_t*)dst, tmp2);
    }

    inline void batch<float, 4>::store_unaligned(int16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(uint16_t* dst) const
    {
        uint32x4_t tmp = vcvtq_u32_f32(this->m_value);
        uint16x4_t tmp2 = vmovn_u32(tmp);
        vst1_u16((uint16_t*)dst, tmp2);
    }

    inline void batch<float, 4>::store_unaligned(uint16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(int32_t* dst) const
    {
        vst1q_s32(dst, vcvtq_s32_f32(this->m_value));
    }

    inline void batch<float, 4>::store_unaligned(int32_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(uint32_t* dst) const
    {
        vst1q_u32(dst, vcvtq_u32_f32(this->m_value));
    }

    inline void batch<float, 4>::store_unaligned(uint32_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(int64_t* dst) const
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        int64x2_t tmp_l = vcvtq_s64_f64(vcvt_f64_f32(vget_low_f32(this->m_value)));
        int64x2_t tmp_h = vcvtq_s64_f64(vcvt_f64_f32(vget_high_f32(this->m_value)));
        vst1q_s64(&(dst[0]), tmp_l);
        vst1q_s64(&(dst[2]), tmp_h);
    #else
        dst[0] = static_cast<int64_t>(this->m_value[0]);
        dst[1] = static_cast<int64_t>(this->m_value[1]);
        dst[2] = static_cast<int64_t>(this->m_value[2]);
        dst[3] = static_cast<int64_t>(this->m_value[3]);
    #endif
    }

    inline void batch<float, 4>::store_unaligned(int64_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(uint64_t* dst) const
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        uint64x2_t tmp_l = vcvtq_u64_f64(vcvt_f64_f32(vget_low_f32(this->m_value)));
        uint64x2_t tmp_h = vcvtq_u64_f64(vcvt_f64_f32(vget_high_f32(this->m_value)));
        vst1q_u64(&(dst[0]), tmp_l);
        vst1q_u64(&(dst[2]), tmp_h);
    #else
        dst[0] = static_cast<uint64_t>(this->m_value[0]);
        dst[1] = static_cast<uint64_t>(this->m_value[1]);
        dst[2] = static_cast<uint64_t>(this->m_value[2]);
        dst[3] = static_cast<uint64_t>(this->m_value[3]);
    #endif
    }

    inline void batch<float, 4>::store_unaligned(uint64_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(float* dst) const
    {
        vst1q_f32(dst, this->m_value);
    }

    inline void batch<float, 4>::store_unaligned(float* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(double* dst) const
    {
    #if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        float64x2_t tmp_l = vcvt_f64_f32(vget_low_f32(this->m_value));
        float64x2_t tmp_h = vcvt_f64_f32(vget_high_f32(this->m_value));
        vst1q_f64(&(dst[0]), tmp_l);
        vst1q_f64(&(dst[2]), tmp_h);
    #else
        dst[0] = static_cast<double>(this->m_value[0]);
        dst[1] = static_cast<double>(this->m_value[1]);
        dst[2] = static_cast<double>(this->m_value[2]);
        dst[3] = static_cast<double>(this->m_value[3]);
    #endif
    }

    inline void batch<float, 4>::store_unaligned(double* dst) const
    {
        store_aligned(dst);
    }

    inline batch<float, 4>::operator float32x4_t() const
    {
        return this->m_value;
    }

    namespace detail
    {
        template <>
        struct batch_kernel<float, 4>
        {
            using batch_type = batch<float, 4>;
            using value_type = float;
            using batch_bool_type = batch_bool<float, 4>;

            static batch_type neg(const batch_type& rhs)
            {
                return vnegq_f32(rhs);
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return vaddq_f32(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return vsubq_f32(lhs, rhs);
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
                return vmulq_f32(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vdivq_f32(lhs, rhs);
#else
                // from stackoverflow & https://projectne10.github.io/Ne10/doc/NE10__divc_8neon_8c_source.html
                // get an initial estimate of 1/b.
                float32x4_t reciprocal = vrecpeq_f32(rhs);

                // use a couple Newton-Raphson steps to refine the estimate.  Depending on your
                // application's accuracy requirements, you may be able to get away with only
                // one refinement (instead of the two used here).  Be sure to test!
                reciprocal = vmulq_f32(vrecpsq_f32(rhs, reciprocal), reciprocal);
                reciprocal = vmulq_f32(vrecpsq_f32(rhs, reciprocal), reciprocal);

                // and finally, compute a / b = a * (1 / b)
                return vmulq_f32(lhs, reciprocal);
#endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return vceqq_f32(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return !(lhs == rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return vcltq_f32(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return vcleq_f32(lhs, rhs);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(lhs),
                                                       vreinterpretq_u32_f32(rhs)));
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(lhs),
                                                       vreinterpretq_u32_f32(rhs)));
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(lhs),
                                                       vreinterpretq_u32_f32(rhs)));
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return vreinterpretq_f32_u32(vmvnq_u32(vreinterpretq_u32_f32(rhs)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(lhs), vreinterpretq_u32_f32(rhs)));
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return vminq_f32(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return vmaxq_f32(lhs, rhs);
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
                return vabsq_f32(rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vsqrtq_f32(rhs);
#else
                batch<float, 4> sqrt_reciprocal = vrsqrteq_f32(rhs);
                // one iter
                sqrt_reciprocal = sqrt_reciprocal * batch<float, 4>(vrsqrtsq_f32(rhs * sqrt_reciprocal, sqrt_reciprocal));
                batch<float, 4> sqrt_approx = rhs * sqrt_reciprocal * batch<float, 4>(vrsqrtsq_f32(rhs * sqrt_reciprocal, sqrt_reciprocal));
                batch<float, 4> zero(0.f);
                return select(rhs == zero, zero, sqrt_approx);
#endif
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#ifdef __ARM_FEATURE_FMA
                return vfmaq_f32(z, x, y);
#else
                return x * y + z;
#endif
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#ifdef __ARM_FEATURE_FMA
                return vfmaq_f32(-z, x, y);
#else
                return x * y - z;
#endif
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
                return vaddvq_f32(rhs);
#else
                float32x2_t tmp = vpadd_f32(vget_low_f32(rhs), vget_high_f32(rhs));
                tmp = vpadd_f32(tmp, tmp);
                return vget_lane_f32(tmp, 0);
#endif
            }

            static batch_type haddp(const simd_batch<batch_type>* row)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                float32x4_t tmp1 = vpaddq_f32(row[0](), row[1]());
                float32x4_t tmp2 = vpaddq_f32(row[2](), row[3]());
                return vpaddq_f32(tmp1, tmp2);
#else
                // row = (a,b,c,d)
                float32x2_t tmp1, tmp2, tmp3;
                // tmp1 = (a0 + a2, a1 + a3)
                tmp1 = vpadd_f32(vget_low_f32(row[0]()), vget_high_f32(row[0]()));
                // tmp2 = (b0 + b2, b1 + b3)
                tmp2 = vpadd_f32(vget_low_f32(row[1]()), vget_high_f32(row[1]()));
                // tmp1 = (a0..3, b0..3)
                tmp1 = vpadd_f32(tmp1, tmp2);
                // tmp2 = (c0 + c2, c1 + c3)
                tmp2 = vpadd_f32(vget_low_f32(row[2]()), vget_high_f32(row[2]()));
                // tmp3 = (d0 + d2, d1 + d3)
                tmp3 = vpadd_f32(vget_low_f32(row[3]()), vget_high_f32(row[3]()));
                // tmp1 = (c0..3, d0..3)
                tmp2 = vpadd_f32(tmp2, tmp3);
                // return = (a0..3, b0..3, c0..3, d0..3)
                return vcombine_f32(tmp1, tmp2);
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return vbslq_f32(cond, a, b);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip1q_f32(lhs, rhs);
#else
                float32x2x2_t tmp = vzip_f32(vget_low_f32(lhs), vget_low_f32(rhs));
                return vcombine_f32(tmp.val[0], tmp.val[1]);
#endif
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
                return vzip2q_f32(lhs, rhs);
#else
                float32x2x2_t tmp = vzip_f32(vget_high_f32(lhs), vget_high_f32(rhs));
                return vcombine_f32(tmp.val[0], tmp.val[1]);
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
