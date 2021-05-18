/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_COMPLEX_HPP
#define XSIMD_SSE_COMPLEX_HPP

#include <complex>

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

#include "xsimd_sse_float.hpp"
#include "xsimd_sse_double.hpp"
#include "xsimd_complex_base.hpp"

namespace xsimd
{

    /**************************************
     * batch_bool<std::complex<float>, 4> *
     **************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<float>, 4>>
        : complex_batch_bool_traits<std::complex<float>, float, 4, 16>
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

    /*********************************
     * batch<std::complex<float>, 4> *
     *********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<float>, 4>>
        : complex_batch_traits<std::complex<float>, float, 4, 16>
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

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<std::complex<float>, 4>>;
    };

    /***************************************
     * batch_bool<std::complex<double>, 2> *
     ***************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<double>, 2>>
        : complex_batch_bool_traits<std::complex<double>, double, 2, 16>
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

    /**********************************
     * batch<std::complex<double>, 2> *
     **********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<double>, 2>>
        : complex_batch_traits<std::complex<double>, double, 2, 16>
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

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<std::complex<double>, 2>>;
    };

    /********************************************
     * batch<std::complex<T>, N> implementation *
     ********************************************/

    inline batch<std::complex<float>, 4>&
    batch<std::complex<float>, 4>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        this->m_real = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 2, 0));
        this->m_imag = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 3, 1));
        return *this;
    }

    inline auto batch<std::complex<float>, 4>::get_complex_high() const -> real_batch
    {
        return _mm_unpacklo_ps(this->m_real, this->m_imag);
    }

    inline auto batch<std::complex<float>, 4>::get_complex_low() const -> real_batch
    {
        return _mm_unpackhi_ps(this->m_real, this->m_imag);
    }

    inline batch<std::complex<double>, 2>&
    batch<std::complex<double>, 2>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        this->m_real = _mm_shuffle_pd(hi, lo, _MM_SHUFFLE2(0, 0));
        this->m_imag = _mm_shuffle_pd(hi, lo, _MM_SHUFFLE2(1, 1));
        return *this;
    }

    inline auto batch<std::complex<double>, 2>::get_complex_high() const -> real_batch
    {
        return _mm_unpacklo_pd(this->m_real, this->m_imag);
    }

    inline auto batch<std::complex<double>, 2>::get_complex_low() const -> real_batch
    {
        return _mm_unpackhi_pd(this->m_real, this->m_imag);
    }

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

        using self_type = batch_bool<xtl::xcomplex<float, float, i3ec>, 4>;
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

        using self_type = batch<xtl::xcomplex<float, float, i3ec>, 4>;
        using base_type = simd_complex_batch<self_type>;
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

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<xtl::xcomplex<float, float, i3ec>, 4>>;
    };

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

    private:

        batch& load_complex(const real_batch& hi, const real_batch& lo);
        real_batch get_complex_high() const;
        real_batch get_complex_low() const;

        friend class simd_complex_batch<batch<xtl::xcomplex<double, double, i3ec>, 2>>;
    };

    /***********************************************************
     * batch<std::xcomplex<T, T, bool i3ec>, N> implementation *
     ***********************************************************/

    template <bool i3ec>
    inline batch<xtl::xcomplex<float, float, i3ec>, 4>&
    batch<xtl::xcomplex<float, float, i3ec>, 4>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        this->m_real = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 2, 0));
        this->m_imag = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 3, 1));
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 4>::get_complex_high() const -> real_batch
    {
        return _mm_unpacklo_ps(this->m_real, this->m_imag);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 4>::get_complex_low() const -> real_batch
    {
        return _mm_unpackhi_ps(this->m_real, this->m_imag);
    }

    template <bool i3ec>
    inline batch<xtl::xcomplex<double, double, i3ec>, 2>&
    batch<xtl::xcomplex<double, double, i3ec>, 2>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        this->m_real = _mm_shuffle_pd(hi, lo, _MM_SHUFFLE2(0, 0));
        this->m_imag = _mm_shuffle_pd(hi, lo, _MM_SHUFFLE2(1, 1));
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 2>::get_complex_high() const -> real_batch
    {
        return _mm_unpacklo_pd(this->m_real, this->m_imag);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 2>::get_complex_low() const -> real_batch
    {
        return _mm_unpackhi_pd(this->m_real, this->m_imag);
    }

#endif

}

#endif
