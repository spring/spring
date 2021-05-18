/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_COMPLEX_HPP
#define XSIMD_NEON_COMPLEX_HPP

#include <complex>
#include <utility>

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

#include "xsimd_neon_float.hpp"
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
#include "xsimd_neon_double.hpp"
#endif
#include "xsimd_complex_base.hpp"

namespace xsimd
{

    /**************************************
     * batch_bool<std::complex<float>, 4> *
     **************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<float>, 4>>
        : complex_batch_bool_traits<std::complex<float>, float, 4, 32>
    {
    };

    template<>
    class batch_bool<std::complex<float>, 4>
        : public simd_complex_batch_bool<batch_bool<std::complex<float>, 4>>
    {
    public:

        using self_type = batch_bool<std::complex<float>, 4>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<float, 4>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3)
            : base_type(real_batch(b0, b1, b2, b3))
        {
        }
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<std::complex<float>, 4>
            : batch_bool_complex_kernel<std::complex<float>, 4>
        {
        };
    }

    /*********************************
     * batch<std::complex<float>, 4> *
     *********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<float>, 4>>
        : complex_batch_traits<std::complex<float>, float, 4, 32>
    {
    };

    template <>
    class batch<std::complex<float>, 4>
        : public simd_complex_batch<batch<std::complex<float>, 4>>
    {
    public:
        using self_type = batch<std::complex<float>, 4>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<float>;
        using real_batch = batch<float, 4>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1, value_type c2, value_type c3)
            : base_type(real_batch(c0.real(), c1.real(), c2.real(), c3.real()),
                        real_batch(c0.imag(), c1.imag(), c2.imag(), c3.imag()))
        {
        }

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        self_type& load_aligned(const std::complex<float>* src);
        self_type& load_unaligned(const std::complex<float>* src);

        self_type& load_aligned(const std::complex<double>* src);
        self_type& load_unaligned(const std::complex<double>* src);

        void store_aligned(std::complex<float>* dst) const;
        void store_unaligned(std::complex<float>* dst) const;

        void store_aligned(std::complex<double>* dst) const;
        void store_unaligned(std::complex<double>* dst) const;
    };

    /************************************************
     * batch<std::complex<float>, 4> implementation *
     ************************************************/
  
    inline auto batch<std::complex<float>, 4>::load_aligned(const std::complex<float>* src) -> self_type&
    {
        const float* buf = reinterpret_cast<const float*>(src);
        float32x4x2_t tmp = vld2q_f32(buf);
        this->m_real = tmp.val[0];
        this->m_imag = tmp.val[1];
        return *this;
    }

    inline auto batch<std::complex<float>, 4>::load_unaligned(const std::complex<float>* src) -> self_type&
    {
        return load_aligned(src);
    }

    inline auto batch<std::complex<float>, 4>::load_aligned(const std::complex<double>* src) -> self_type&
    {
        const double* buf = reinterpret_cast<const double*>(src);
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        float64x2x2_t tmp0 = vld2q_f64(buf);
        float64x2x2_t tmp1 = vld2q_f64(buf + 4);
        float32x2x2_t tmp2 { vcvt_f32_f64(tmp0.val[0]), vcvt_f32_f64(tmp0.val[1]) };
        float32x2x2_t tmp3 { vcvt_f32_f64(tmp1.val[0]), vcvt_f32_f64(tmp1.val[1]) };
        this->m_real = vcombine_f32(tmp2.val[0], tmp3.val[0]);
        this->m_imag = vcombine_f32(tmp2.val[1], tmp3.val[1]);
#else
        this->m_real = real_batch(static_cast<float>(buf[0]),
                                  static_cast<float>(buf[2]),
                                  static_cast<float>(buf[4]),
                                  static_cast<float>(buf[6]));
        this->m_imag = real_batch(static_cast<float>(buf[1]),
                                  static_cast<float>(buf[3]),
                                  static_cast<float>(buf[5]),
                                  static_cast<float>(buf[7]));
#endif
        return *this;
    }

    inline auto batch<std::complex<float>, 4>::load_unaligned(const std::complex<double>* src) -> self_type&
    {
        return load_aligned(src);
    }

    inline void batch<std::complex<float>, 4>::store_aligned(std::complex<float>* dst) const
    {
        float32x4x2_t tmp;
        tmp.val[0] = this->m_real;
        tmp.val[1] = this->m_imag;
        float* buf = reinterpret_cast<float*>(dst);
        vst2q_f32(buf, tmp);
    }

    inline void batch<std::complex<float>, 4>::store_unaligned(std::complex<float>* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<std::complex<float>, 4>::store_aligned(std::complex<double>* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        double* buf = reinterpret_cast<double*>(dst);
        float64x2x2_t tmp0 { vcvt_f64_f32(vget_low_f32(this->m_real)),
                             vcvt_f64_f32(vget_low_f32(this->m_imag)) };
        float64x2x2_t tmp1 { vcvt_f64_f32(vget_high_f32(this->m_real)),
                             vcvt_f64_f32(vget_high_f32(this->m_imag)) };
        vst2q_f64(buf, tmp0);
        vst2q_f64(buf + 4, tmp1);
#else
        for (std::size_t i = 0; i < 4; ++i)
        {
            dst[i] = std::complex<double>(this->m_real[i], this->m_imag[i]);
        }
#endif
    }

    inline void batch<std::complex<float>, 4>::store_unaligned(std::complex<double>* dst) const
    {
        store_aligned(dst);
    }

#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION

    /***************************************
     * batch_bool<std::complex<double>, 2> *
     ***************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<double>, 2>>
        : complex_batch_bool_traits<std::complex<double>, double, 2, 32>
    {
    };

    template<>
    class batch_bool<std::complex<double>, 2>
        : public simd_complex_batch_bool<batch_bool<std::complex<double>, 2>>
    {
    public:

        using self_type = batch_bool<std::complex<double>, 2>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<double, 2>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1)
            : base_type(real_batch(b0, b1))
        {
        }
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<std::complex<double>, 2>
            : batch_bool_complex_kernel<std::complex<double>, 2>
        {
        };
    }

    /**********************************
     * batch<std::complex<double>, 2> *
     **********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<double>, 2>>
        : complex_batch_traits<std::complex<double>, double, 2, 32>
    {
    };

    template <>
    class batch<std::complex<double>, 2>
        : public simd_complex_batch<batch<std::complex<double>, 2>>
    {
    public:

        using self_type = batch<std::complex<double>, 2>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<double>;
        using real_batch = batch<double, 2>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1)
            : base_type(real_batch(c0.real(), c1.real()),
                        real_batch(c0.imag(), c1.imag()))
        {
        }

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        self_type& load_aligned(const std::complex<float>* src);
        self_type& load_unaligned(const std::complex<float>* src);

        self_type& load_aligned(const std::complex<double>* src);
        self_type& load_unaligned(const std::complex<double>* src);

        void store_aligned(std::complex<float>* dst) const;
        void store_unaligned(std::complex<float>* dst) const;

        void store_aligned(std::complex<double>* dst) const;
        void store_unaligned(std::complex<double>* dst) const;
    };

    /*************************************************
     * batch<std::complex<double>, 2> implementation *
     *************************************************/

    inline auto batch<std::complex<double>, 2>::load_aligned(const std::complex<float>* src) -> self_type&
    {
        const float* buf = reinterpret_cast<const float*>(src);
        float32x2x2_t tmp = vld2_f32(buf);
        this->m_real = vcvt_f64_f32(tmp.val[0]);
        this->m_imag = vcvt_f64_f32(tmp.val[1]);
        return *this;
    }

    inline auto batch<std::complex<double>, 2>::load_unaligned(const std::complex<float>* src) -> self_type&
    {
        return load_aligned(src);
    }

    inline auto batch<std::complex<double>, 2>::load_aligned(const std::complex<double>* src) -> self_type&
    {
        const double* buf = reinterpret_cast<const double*>(src);
        float64x2x2_t tmp = vld2q_f64(buf);
        this->m_real = tmp.val[0];
        this->m_imag = tmp.val[1];
        return *this;
    }

    inline auto batch<std::complex<double>, 2>::load_unaligned(const std::complex<double>* src) -> self_type&
    {
        return load_aligned(src);
    }

    inline void batch<std::complex<double>, 2>::store_aligned(std::complex<float>* dst) const
    {
        float* buf = reinterpret_cast<float*>(dst);
        float32x2x2_t tmp { vcvt_f32_f64(this->m_real), vcvt_f32_f64(this->m_imag) };
        vst2_f32(buf, tmp);
    }

    inline void batch<std::complex<double>, 2>::store_unaligned(std::complex<float>* dst) const
    {
        return store_aligned(dst);
    }

    inline void batch<std::complex<double>, 2>::store_aligned(std::complex<double>* dst) const
    {
        float64x2x2_t tmp;
        tmp.val[0] = this->m_real;
        tmp.val[1] = this->m_imag;
        double* buf = reinterpret_cast<double*>(dst);
        vst2q_f64(buf, tmp);
    }

    inline void batch<std::complex<double>, 2>::store_unaligned(std::complex<double>* dst) const
    {
        return store_aligned(dst);
    }

#endif

#ifdef XSIMD_ENABLE_XTL_COMPLEX

    /****************************************************
     * batch_bool<xtl::xcomplex<float, float, i3ec>, 4> *
     ****************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<float, float, i3ec>, 4>>
        : complex_batch_bool_traits<xtl::xcomplex<float, float, i3ec>, float, 4, 16>
    {
    };

    template<bool i3ec>
    class batch_bool<xtl::xcomplex<float, float, i3ec>, 4>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<float, float, i3ec>, 4>>
    {
    public:

        batch_bool() = default;
        using simd_complex_batch_bool::simd_complex_batch_bool;

        using real_batch = batch_bool<float, 4>;
        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3)
            : simd_complex_batch_bool(real_batch(b0, b1, b2, b3))
        {
        }
    };

    namespace detail
    {
        template <bool i3ec>
        struct batch_bool_kernel<xtl::xcomplex<float, float, i3ec>, 4>
            : batch_bool_complex_kernel<xtl::xcomplex<float, float, i3ec>, 4>
        {
        };
    }

    /***********************************************
     * batch<xtl::xcomplex<float, float, i3ec>, 4> *
     ***********************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<float, float, i3ec>, 4>>
        : complex_batch_traits<xtl::xcomplex<float, float, i3ec>, float, 4, 16>
    {
    };

    template <bool i3ec>
    class batch<xtl::xcomplex<float, float, i3ec>, 4>
        : public simd_complex_batch<batch<xtl::xcomplex<float, float, i3ec>, 4>>
    {
    public:

        using base_type = batch<xtl::xcomplex<float, float, i3ec>, 4>;
        using self_type = simd_complex_batch<base_type>;
        using value_type = xtl::xcomplex<float, float, i3ec>;
        using real_batch = batch<float, 4>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1, value_type c2, value_type c3)
            : base_type(real_batch(c0.real(), c1.real(), c2.real(), c3.real()),
                        real_batch(c0.imag(), c1.imag(), c2.imag(), c3.imag()))
        {
        }

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        self_type& load_aligned(const std::complex<float>* src);
        self_type& load_unaligned(const std::complex<float>* src);

        self_type& load_aligned(const std::complex<double>* src);
        self_type& load_unaligned(const std::complex<double>* src);

        void store_aligned(std::complex<float>* dst) const;
        void store_unaligned(std::complex<float>* dst) const;

        void store_aligned(std::complex<double>* dst) const;
        void store_unaligned(std::complex<double>* dst) const;
    };

    /*******************************************************************
     * batch<std::xcomplex<float, float, bool i3ec>, 4> implementation *
     *******************************************************************/

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 4>::load_aligned(const value_type* src) -> self_type&
    {
        const float* buf = reinterpret_cast<const float*>(src);
        float32x4x2_t tmp = vld2q_f32(buf);
        this->m_real = tmp.val[0];
        this->m_imag = tmp.val[1];
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 4>::load_unaligned(const value_type* src) -> self_type&
    {
        return load_aligned(src);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 4>::load_aligned(const xtl::complex<double, double, i3ec>* src) -> self_type&
    {
        const double* buf = reinterpret_cast<const double*>(src);
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        float64x2x2_t tmp0 = vld2q_f64(buf);
        float64x2x2_t tmp1 = vld2q_f64(buf + 4);
        float32x2x2_t tmp2{ vcvt_f32_f64(tmp0.val[0]), vcvt_f32_f64(tmp0.val[1]) };
        float32x2x2_t tmp3{ vcvt_f32_f64(tmp1.val[0]), vcvt_f32_f64(tmp1.val[1]) };
        this->m_real = vcombine_f32(tmp2.val[0], tmp3.val[0]);
        this->m_imag = vcombine_f32(tmp2.val[1], tmp3.val[1]);
#else
        this->m_real = real_batch(static_cast<float>(buf[0]),
            static_cast<float>(buf[2]),
            static_cast<float>(buf[4]),
            static_cast<float>(buf[6]));
        this->m_imag = real_batch(static_cast<float>(buf[1]),
            static_cast<float>(buf[3]),
            static_cast<float>(buf[5]),
            static_cast<float>(buf[7]));
#endif
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 4>::load_unaligned(const xtl::complex<double, double, i3ec>* src) -> self_type&
    {
        return load_unaligned(src);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<float, float, i3ec>, 4>::store_aligned(value_type* dst) const
    {
        float32x4x2_t tmp;
        tmp.val[0] = this->m_real;
        tmp.val[1] = this->m_imag;
        float* buf = reinterpret_cast<float*>(dst);
        vst2q_f32(buf, tmp);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<float, float, i3ec>, 4>::store_unaligned(value_type* dst) const
    {
        store_aligned(dst);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<float, float, i3ec, 4>::store_aligned(xtl::xcomplex<double, double, i3ec>* dst) const
    {
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
        double* buf = reinterpret_cast<double*>(dst);
        float64x2x2_t tmp0{ vcvt_f64_f32(vget_low_f32(this->m_real)),
            vcvt_f64_f32(vget_low_f32(this->m_imag)) };
        float64x2x2_t tmp1{ vcvt_f64_f32(vget_high_f32(this->m_real)),
            vcvt_f64_f32(vget_high_f32(this->m_imag)) };
        vst2q_f64(buf, tmp0);
        vst2q_f64(buf + 4, tmp1);
#else
        for (std::size_t i = 0; i < 4; ++i)
        {
            dst[i] = std::complex<double>(this->m_real[i], this->m_imag[i]);
        }
#endif
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<float, float, i3ec>, 4>::store_unaligned(xtl::xcomplex<double, double, i3ec>* dst) const
    {
        store_aligned(dst);
    }

#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION

    /******************************************************
     * batch_bool<xtl::xcomplex<double, double, i3ec>, 2> *
     ******************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<double, double, i3ec>, 2>>
        : complex_batch_bool_traits<xtl::xcomplex<double, double, i3ec>, double, 2, 16>
    {
    };

    template<bool i3ec>
    class batch_bool<xtl::xcomplex<double, double, i3ec>, 2>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<double, double, i3ec>, 2>>
    {
    public:

        using self_type = batch_bool<xtl::xcomplex<double, double, i3ec>, 2>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<double, 2>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1)
            : base_type(real_batch(b0, b1))
        {
        }
    };

    namespace detail
    {
        template <bool i3ec>
        struct batch_bool_kernel<xtl::xcomplex<double, double, i3ec>, 2>
            : batch_bool_complex_kernel<xtl::xcomplex<double, double, i3ec>, 2>
        {
        };
    }

    /*************************************************
     * batch<xtl::xcomplex<double, double, i3ec>, 2> *
     *************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<double, double, i3ec>, 2>>
        : complex_batch_traits<xtl::xcomplex<double, double, i3ec>, double, 2, 16>
    {
    };

    template <bool i3ec>
    class batch<xtl::xcomplex<double, double, i3ec>, 2>
        : public simd_complex_batch<batch<xtl::xcomplex<double, double, i3ec>, 2>>
    {
    public:

        using self_type = batch<xtl::xcomplex<double, double, i3ec>, 2>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = xtl::xcomplex<double, double, i3ec>;
        using real_batch = batch<double, 2>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1)
            : base_type(real_batch(c0.real(), c1.real()),
                        real_batch(c0.imag(), c1.imag()))
        {
        }

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        self_type& load_aligned(const std::complex<float>* src);
        self_type& load_unaligned(const std::complex<float>* src);

        self_type& load_aligned(const std::complex<double>* src);
        self_type& load_unaligned(const std::complex<double>* src);

        void store_aligned(std::complex<float>* dst) const;
        void store_unaligned(std::complex<float>* dst) const;

        void store_aligned(std::complex<double>* dst) const;
        void store_unaligned(std::complex<double>* dst) const;
    };

    /*******************************************************************
     * batch<std::xcomplex<double, double, bool i3ec>, 2> implementation *
     *******************************************************************/

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 2>::load_aligned(const xtl::xcomplex<float, float, i3ec>* src) -> self_type&
    {
        const float* buf = reinterpret_cast<const float*>(src);
        float32x2x2_t tmp = vld2_f32(buf);
        this->m_real = vcvt_f64_f32(tmp.val[0]);
        this->m_imag = vcvt_f64_f32(tmp.val[1]);
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 2>::load_unaligned(const xtl::xcomplex<float, float, i3ec>* src) -> self_type&
    {
        return load_aligned(src);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 2>::load_aligned(const value_type* src) -> self_type&
    {
        const double* buf = reinterpret_cast<const double*>(src);
        float64x2x2_t tmp = vld2q_f64(buf);
        this->m_real = tmp.val[0];
        this->m_imag = tmp.val[1];
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 2>::load_unaligned(const value_type* src) -> self_type&
    {
        return load_aligned(src);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<double, double, i3ec>, 2>::store_aligned(xtl::xcomplex<float, float, i3ec>* dst) const
    {
        float* buf = reinterpret_cast<float*>(dst);
        float32x2x2_t tmp{ vcvt_f32_f64(this->m_real), vcvt_f32_f64(this->m_imag) };
        vst2_f32(buf, tmp);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<double, double, i3ec>, 2>::store_unaligned(xtl::xcomplex<float, float, i3ec>* dst) const
    {
        return store_aligned(dst);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<double, double, i3ec>, 2>::store_aligned(value_type* dst) const
    {
        float64x2x2_t tmp;
        tmp.val[0] = this->m_real;
        tmp.val[1] = this->m_imag;
        double* buf = reinterpret_cast<double*>(dst);
        vst2q_f64(buf, tmp);
    }

    template <bool i3ec>
    inline void batch<xtl::xcomplex<double, double, i3ec>, 2>::store_unaligned(value_type* dst) const
    {
        return store_aligned(dst);
    }

#endif
#endif

}

#endif
