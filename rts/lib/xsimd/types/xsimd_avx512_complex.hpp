/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_COMPLEX_HPP
#define XSIMD_AVX512_COMPLEX_HPP

#include <complex>

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

#include "xsimd_avx512_float.hpp"
#include "xsimd_avx512_double.hpp"
#include "xsimd_complex_base.hpp"

namespace xsimd
{

    /***************************************
     * batch_bool<std::complex<float>, 16> *
     ***************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<float>, 16>>
        : complex_batch_bool_traits<std::complex<float>, float, 16, 64>
    {
    };

    template<>
    class batch_bool<std::complex<float>, 16>
        : public simd_complex_batch_bool<batch_bool<std::complex<float>, 16>>
    {
    public:

        using self_type = batch_bool<std::complex<float>, 16>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<float, 16>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7,
                   bool b8, bool b9, bool b10, bool b11, bool b12, bool b13, bool b14, bool b15)
            : base_type(real_batch(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15))
        {
        }
    };

    /**********************************
     * batch<std::complex<float>, 16> *
     **********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<float>, 16>>
        : complex_batch_traits<std::complex<float>, float, 16, 64>
    {
    };

    template <>
    class batch<std::complex<float>, 16>
        : public simd_complex_batch<batch<std::complex<float>, 16>>
    {
    public:

        using self_type = batch<std::complex<float>, 16>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<float>;
        using real_batch = batch<float, 16>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1, value_type c2, value_type c3,
              value_type c4, value_type c5, value_type c6, value_type c7,
              value_type c8, value_type c9, value_type c10, value_type c11,
              value_type c12, value_type c13, value_type c14, value_type c15)
            : base_type(real_batch(c0.real(), c1.real(), c2.real(), c3.real(), c4.real(), c5.real(), c6.real(), c7.real(),
                                   c8.real(), c9.real(), c10.real(), c11.real(), c12.real(), c13.real(), c14.real(), c15.real()),
                        real_batch(c0.imag(), c1.imag(), c2.imag(), c3.imag(), c4.imag(), c5.imag(), c6.imag(), c7.imag(),
                                   c8.imag(), c9.imag(), c10.imag(), c11.imag(), c12.imag(), c13.imag(), c14.imag(), c15.imag()))
        {
        }

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<std::complex<float>, 16>>;
    };

    /***************************************
     * batch_bool<std::complex<double>, 8> *
     ***************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<double>, 8>>
        : complex_batch_bool_traits<std::complex<double>, double, 8, 64>
    {
    };

    template<>
    class batch_bool<std::complex<double>, 8>
        : public simd_complex_batch_bool<batch_bool<std::complex<double>, 8>>
    {
    public:

        using self_type = batch_bool<std::complex<double>, 8>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<double, 8>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
            : base_type(real_batch(b0, b1, b2, b3, b4, b5, b6, b7))
        {
        }
    };

    /**********************************
     * batch<std::complex<double>, 8> *
     **********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<double>, 8>>
        : complex_batch_traits<std::complex<double>, double, 8, 64>
    {
    };

    template <>
    class batch<std::complex<double>, 8>
        : public simd_complex_batch<batch<std::complex<double>, 8>>
    {
    public:

        using self_type = batch<std::complex<double>, 8>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<double>;
        using real_batch = batch<double, 8>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1, value_type c2, value_type c3,
              value_type c4, value_type c5, value_type c6, value_type c7)
            : base_type(real_batch(c0.real(), c1.real(), c2.real(), c3.real(), c4.real(), c5.real(), c6.real(), c7.real()),
                        real_batch(c0.imag(), c1.imag(), c2.imag(), c3.imag(), c4.imag(), c5.imag(), c6.imag(), c7.imag()))
        {
        }

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<std::complex<double>, 8>>;
    };

    /********************************************
     * batch<std::complex<T>, N> implementation *
     ********************************************/

    inline batch<std::complex<float>, 16>&
    batch<std::complex<float>, 16>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        __m512i real_idx = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
        __m512i imag_idx = _mm512_setr_epi32(1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
        this->m_real = _mm512_permutex2var_ps(hi, real_idx, lo);
        this->m_imag = _mm512_permutex2var_ps(hi, imag_idx, lo);
        return *this;
    }

    inline auto batch<std::complex<float>, 16>::get_complex_high() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi32(0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
        return _mm512_permutex2var_ps(this->m_real, idx, this->m_imag);
    }

    inline auto batch<std::complex<float>, 16>::get_complex_low() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi32(8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
        return _mm512_permutex2var_ps(this->m_real, idx, this->m_imag);
    }

    inline batch<std::complex<double>, 8>&
    batch<std::complex<double>, 8>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        __m512i real_idx = _mm512_setr_epi64(0, 2, 4, 6, 8, 10, 12, 14);
        __m512i imag_idx = _mm512_setr_epi64(1, 3, 5, 7, 9, 11, 13, 15);
        this->m_real = _mm512_permutex2var_pd(hi, real_idx, lo);
        this->m_imag = _mm512_permutex2var_pd(hi, imag_idx, lo);
        return *this;
    }

    inline auto batch<std::complex<double>, 8>::get_complex_high() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi64(0, 8, 1, 9, 2, 10, 3, 11);
        return _mm512_permutex2var_pd(this->m_real, idx, this->m_imag);
    }

    inline auto batch<std::complex<double>, 8>::get_complex_low() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi64(4, 12, 5, 13, 6, 14, 7, 15);
        return _mm512_permutex2var_pd(this->m_real, idx, this->m_imag);
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX

    /*****************************************************
     * batch_bool<xtl::xcomplex<float, float, i3ec>, 16> *
     *****************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<float, float, i3ec>, 16>>
        : complex_batch_bool_traits<xtl::xcomplex<float, float, i3ec>, float, 16, 64>
    {
    };

    template<bool i3ec>
    class batch_bool<xtl::xcomplex<float, float, i3ec>, 16>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<float, float, i3ec>, 16>>
    {
    public:

        using self_type = batch_bool<xtl::xcomplex<float, float, i3ec>, 16>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<float, 16>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7,
                   bool b8, bool b9, bool b10, bool b11, bool b12, bool b13, bool b14, bool b15)
            : base_type(real_batch(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15))
        {
        }
    };

    /***********************************************
     * batch<xtl::xcomplex<float, float, i3ec>, 16> *
     ***********************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<float, float, i3ec>, 16>>
        : complex_batch_traits<xtl::xcomplex<float, float, i3ec>, float, 16, 64>
    {
    };

    template <bool i3ec>
    class batch<xtl::xcomplex<float, float, i3ec>, 16>
        : public simd_complex_batch<batch<xtl::xcomplex<float, float, i3ec>, 16>>
    {
    public:

        using self_type = batch<xtl::xcomplex<float, float, i3ec>, 16>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = xtl::xcomplex<float, float, i3ec>;
        using real_batch = batch<float, 16>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1, value_type c2, value_type c3,
              value_type c4, value_type c5, value_type c6, value_type c7,
              value_type c8, value_type c9, value_type c10, value_type c11,
              value_type c12, value_type c13, value_type c14, value_type c15)
            : base_type(real_batch(c0.real(), c1.real(), c2.real(), c3.real(), c4.real(), c5.real(), c6.real(), c7.real(),
                                   c8.real(), c9.real(), c10.real(), c11.real(), c12.real(), c13.real(), c14.real(), c15.real()),
                        real_batch(c0.imag(), c1.imag(), c2.imag(), c3.imag(), c4.imag(), c5.imag(), c6.imag(), c7.imag(),
                                   c8.imag(), c9.imag(), c10.imag(), c11.imag(), c12.imag(), c13.imag(), c14.imag(), c15.imag()))
        {
        }

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<xtl::xcomplex<float, float, i3ec>, 16>>;
    };

    /******************************************************
     * batch_bool<xtl::xcomplex<double, double, i3ec>, 8> *
     ******************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<double, double, i3ec>, 8>>
        : complex_batch_bool_traits<xtl::xcomplex<double, double, i3ec>, double, 8, 64>
    {
    };

    template<bool i3ec>
    class batch_bool<xtl::xcomplex<double, double, i3ec>, 8>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<double, double, i3ec>, 8>>
    {
    public:

        using self_type = batch_bool<xtl::xcomplex<double, double, i3ec>, 8>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<double, 8>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
            : base_type(real_batch(b0, b1, b2, b3, b4, b5, b6, b7))
        {
        }
    };

    /*************************************************
     * batch<xtl::xcomplex<double, double, i3ec>, 8> *
     *************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<double, double, i3ec>, 8>>
        : complex_batch_traits<xtl::xcomplex<double, double, i3ec>, double, 8, 64>
    {
    };

    template <bool i3ec>
    class batch<xtl::xcomplex<double, double, i3ec>, 8>
        : public simd_complex_batch<batch<xtl::xcomplex<double, double, i3ec>, 8>>
    {
    public:

        using self_type = batch<xtl::xcomplex<double, double, i3ec>, 8>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = xtl::xcomplex<double, double, i3ec>;
        using real_batch = batch<double, 8>;

        batch() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch(value_type c0, value_type c1, value_type c2, value_type c3,
              value_type c4, value_type c5, value_type c6, value_type c7)
            : base_type(real_batch(c0.real(), c1.real(), c2.real(), c3.real(), c4.real(), c5.real(), c6.real(), c7.real()),
                        real_batch(c0.imag(), c1.imag(), c2.imag(), c3.imag(), c4.imag(), c5.imag(), c6.imag(), c7.imag()))
        {
        }

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<xtl::xcomplex<double, double, i3ec>, 8>>;
    };

    /********************************************
     * batch<std::complex<T>, N> implementation *
     ********************************************/

    template <bool i3ec>
    inline batch<xtl::xcomplex<float, float, i3ec>, 16>&
    batch<xtl::xcomplex<float, float, i3ec>, 16>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        __m512i real_idx = _mm512_setr_epi32(0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
        __m512i imag_idx = _mm512_setr_epi32(1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
        this->m_real = _mm512_permutex2var_ps(hi, real_idx, lo);
        this->m_imag = _mm512_permutex2var_ps(hi, imag_idx, lo);
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 16>::get_complex_high() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi32(0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
        return _mm512_permutex2var_ps(this->m_real, idx, this->m_imag);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 16>::get_complex_low() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi32(8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
        return _mm512_permutex2var_ps(this->m_real, idx, this->m_imag);
    }

    template <bool i3ec>
    inline batch<xtl::xcomplex<double, double, i3ec>, 8>&
    batch<xtl::xcomplex<double, double, i3ec>, 8>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        __m512i real_idx = _mm512_setr_epi64(0, 2, 4, 6, 8, 10, 12, 14);
        __m512i imag_idx = _mm512_setr_epi64(1, 3, 5, 7, 9, 11, 13, 15);
        this->m_real = _mm512_permutex2var_pd(hi, real_idx, lo);
        this->m_imag = _mm512_permutex2var_pd(hi, imag_idx, lo);
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 8>::get_complex_high() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi64(0, 8, 1, 9, 2, 10, 3, 11);
        return _mm512_permutex2var_pd(this->m_real, idx, this->m_imag);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 8>::get_complex_low() const -> real_batch
    {
        __m512i idx = _mm512_setr_epi64(4, 12, 5, 13, 6, 14, 7, 15);
        return _mm512_permutex2var_pd(this->m_real, idx, this->m_imag);
    }

#endif
}

#endif
