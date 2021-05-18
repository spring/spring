/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_COMPLEX_HPP
#define XSIMD_AVX_COMPLEX_HPP

#include <complex>
#include <tuple>
#include <utility>

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

#include "xsimd_avx_float.hpp"
#include "xsimd_avx_double.hpp"
#include "xsimd_complex_base.hpp"

namespace xsimd
{

    /**************************************
     * batch_bool<std::complex<float>, 8> *
     **************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<float>, 8>>
        : complex_batch_bool_traits<std::complex<float>, float, 8, 32>
    {
    };

    template<>
    class batch_bool<std::complex<float>, 8>
        : public simd_complex_batch_bool<batch_bool<std::complex<float>, 8>>
    {
    public:

        using self_type = batch_bool<std::complex<float>, 8>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<float, 8>;

        batch_bool() = default;
        using base_type::base_type;
        
        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
            : base_type(real_batch(b0, b1, b2, b3, b4, b5, b6, b7))
        {
        }
    };

    /*********************************
     * batch<std::complex<float>, 8> *
     *********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<float>, 8>>
        : complex_batch_traits<std::complex<float>, float, 8, 32>
    {
    };

    template <>
    class batch<std::complex<float>, 8>
        : public simd_complex_batch<batch<std::complex<float>, 8>>
    {
    public:

        using self_type = batch<std::complex<float>, 8>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<float>;
        using real_batch = batch<float, 8>;

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

        friend class simd_complex_batch<batch<std::complex<float>, 8>>;
    };

    /***************************************
     * batch_bool<std::complex<double>, 4> *
     ***************************************/

    template <>
    struct simd_batch_traits<batch_bool<std::complex<double>, 4>>
        : complex_batch_bool_traits<std::complex<double>, double, 4, 32>
    {
    };

    template<>
    class batch_bool<std::complex<double>, 4>
        : public simd_complex_batch_bool<batch_bool<std::complex<double>, 4>>
    {
    public:

        using self_type = batch_bool<std::complex<double>, 4>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<double, 4>;

        batch_bool() = default;
        using base_type::base_type;
        
        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3)
            : base_type(real_batch(b0, b1, b2, b3))
        {
        }
    };

    /**********************************
     * batch<std::complex<double>, 4> *
     **********************************/

    template <>
    struct simd_batch_traits<batch<std::complex<double>, 4>>
        : complex_batch_traits<std::complex<double>, double, 4, 32>
    {
    };

    template <>
    class batch<std::complex<double>, 4>
        : public simd_complex_batch<batch<std::complex<double>, 4>>
    {
    public:

        using self_type = batch<std::complex<double>, 4>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = std::complex<double>;
        using real_batch = batch<double, 4>;

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

        friend class simd_complex_batch<batch<std::complex<double>, 4>>;
    };

    /**********************************************
     * common functions to avoid code duplication *
     **********************************************/

    namespace detail
    {
        template <class B>
        inline std::pair<B, B> load_complex_f(const B& hi, const B& lo)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            B real = _mm256_castpd_ps(
                         _mm256_permute4x64_pd(
                             _mm256_castps_pd(_mm256_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 2, 0))),
                             _MM_SHUFFLE(3, 1, 2, 0)));
            B imag = _mm256_castpd_ps(
                         _mm256_permute4x64_pd(
                             _mm256_castps_pd(_mm256_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 3, 1))),
                             _MM_SHUFFLE(3, 1, 2, 0)));

#else
            __m128 tmp0 = _mm256_extractf128_ps(hi, 0);
            __m128 tmp1 = _mm256_extractf128_ps(hi, 1);
            __m128 tmp_real = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(2, 0, 2, 0));
            __m128 tmp_imag = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(3, 1, 3, 1));
            B real, imag;
            real = _mm256_insertf128_ps(real, tmp_real, 0);
            imag = _mm256_insertf128_ps(imag, tmp_imag, 0);
            tmp0 = _mm256_extractf128_ps(lo, 0);
            tmp1 = _mm256_extractf128_ps(lo, 1);
            tmp_real = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(2, 0, 2, 0));
            tmp_imag = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(3, 1, 3, 1));
            real = _mm256_insertf128_ps(real, tmp_real, 1);
            imag = _mm256_insertf128_ps(imag, tmp_imag, 1);
#endif
            return std::make_pair(real, imag);
        }

        // On clang, _mm256_extractf128_ps is built upon build_shufflevector
        // which require index parameter to be a constant
        template <int index, class B>
        inline B get_half_complex_f(const B& real, const B& imag)
        {
            __m128 tmp0 = _mm256_extractf128_ps(real, index);
            __m128 tmp1 = _mm256_extractf128_ps(imag, index);
            __m128 tmp2 = _mm_unpackhi_ps(tmp0, tmp1);
            tmp0 = _mm_unpacklo_ps(tmp0, tmp1);
            __m256 res = real;
            res = _mm256_insertf128_ps(res, tmp0, 0);
            res = _mm256_insertf128_ps(res, tmp2, 1);
            return res;
        }

        template <class B>
        inline B get_complex_high_f(const B& real, const B& imag)
        {
            return get_half_complex_f<0>(real, imag);
        }

        template <class B>
        inline B get_complex_low_f(const B& real, const B& imag)
        {
            return get_half_complex_f<1>(real, imag);
        }

        template <class B>
        inline std::pair<B, B> load_complex_d(const B& hi, const B& lo)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            B real = _mm256_permute4x64_pd(_mm256_unpacklo_pd(hi, lo), _MM_SHUFFLE(3, 1, 2, 0));
            B imag = _mm256_permute4x64_pd(_mm256_unpackhi_pd(hi, lo), _MM_SHUFFLE(3, 1, 2, 0));
#else
            __m128d tmp0 = _mm256_extractf128_pd(hi, 0);
            __m128d tmp1 = _mm256_extractf128_pd(hi, 1);
            B real, imag;
            __m256d re_tmp0 = _mm256_insertf128_pd(real, _mm_unpacklo_pd(tmp0, tmp1), 0);
            __m256d im_tmp0 = _mm256_insertf128_pd(imag, _mm_unpackhi_pd(tmp0, tmp1), 0);
            tmp0 = _mm256_extractf128_pd(lo, 0);
            tmp1 = _mm256_extractf128_pd(lo, 1);
            __m256d re_tmp1 = _mm256_insertf128_pd(real, _mm_unpacklo_pd(tmp0, tmp1), 1);
            __m256d im_tmp1 = _mm256_insertf128_pd(imag, _mm_unpackhi_pd(tmp0, tmp1), 1);
            real = _mm256_blend_pd(re_tmp0, re_tmp1, 12);
            imag = _mm256_blend_pd(im_tmp0, im_tmp1, 12);
#endif
            return std::make_pair(real, imag);
        }

        // On clang, _mm256_extractf128_pd is built upon build_shufflevector
        // which require index parameter to be a constant
        template <int index, class B>
        inline B get_half_complex_d(const B& real, const B& imag)
        {
            __m128d tmp0 = _mm256_extractf128_pd(real, index);
            __m128d tmp1 = _mm256_extractf128_pd(imag, index);
            __m128d tmp2 = _mm_unpackhi_pd(tmp0, tmp1);
            tmp0 = _mm_unpacklo_pd(tmp0, tmp1);
            __m256d res = real;
            res = _mm256_insertf128_pd(res, tmp0, 0);
            res = _mm256_insertf128_pd(res, tmp2, 1);
            return res;
        }

        template <class B>
        inline B get_complex_high_d(const B& real, const B& imag)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256d tmp0 = _mm256_permute4x64_pd(real, _MM_SHUFFLE(3, 1, 1, 0));
            __m256d tmp1 = _mm256_permute4x64_pd(imag, _MM_SHUFFLE(1, 2, 0, 0));
            return _mm256_blend_pd(tmp0, tmp1, 10);
#else
            return get_half_complex_d<0>(real, imag);
#endif
        }

        template <class B>
        inline B get_complex_low_d(const B& real, const B& imag)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256d tmp0 = _mm256_permute4x64_pd(real, _MM_SHUFFLE(3, 3, 1, 2));
            __m256d tmp1 = _mm256_permute4x64_pd(imag, _MM_SHUFFLE(3, 2, 2, 0));
            return _mm256_blend_pd(tmp0, tmp1, 10);
#else
            return get_half_complex_d<1>(real, imag);
#endif
        }
    }

    /********************************************
     * batch<std::complex<T>, N> implementation *
     ********************************************/

    inline batch<std::complex<float>, 8>&
    batch<std::complex<float>, 8>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        std::tie(this->m_real, this->m_imag) = detail::load_complex_f(hi, lo);
        return *this;
    }

    inline auto batch<std::complex<float>, 8>::get_complex_high() const -> real_batch
    {
        return detail::get_complex_high_f(this->m_real, this->m_imag);
    }

    inline auto batch<std::complex<float>, 8>::get_complex_low() const -> real_batch
    {
        return detail::get_complex_low_f(this->m_real, this->m_imag);
    }

    inline batch<std::complex<double>, 4>&
    batch<std::complex<double>, 4>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        std::tie(m_real, m_imag) = detail::load_complex_d(hi, lo);
        return *this;
    }

    inline auto batch<std::complex<double>, 4>::get_complex_high() const -> real_batch
    {
        return detail::get_complex_high_d(this->m_real, this->m_imag);
    }

    inline auto batch<std::complex<double>, 4>::get_complex_low() const -> real_batch
    {
        return detail::get_complex_low_d(this->m_real, this->m_imag);
    }

#ifdef XSIMD_ENABLE_XTL_COMPLEX

    /****************************************************
     * batch_bool<xtl::xcomplex<float, float, i3ec>, 8> *
     ****************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<float, float, i3ec>, 8>>
        : complex_batch_bool_traits<xtl::xcomplex<float, float, i3ec>, float, 8, 32>
    {
    };

    template<bool i3ec>
    class batch_bool<xtl::xcomplex<float, float, i3ec>, 8>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<float, float, i3ec>, 8>>
    {
    public:

        using self_type = batch_bool<xtl::xcomplex<float, float, i3ec>, 8>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<float, 8>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
            : base_type(real_batch(b0, b1, b2, b3, b4, b5, b6, b7))
        {
        }
    };

    /***********************************************
     * batch<xtl::xcomplex<float, float, i3ec>, 8> *
     ***********************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<float, float, i3ec>, 8>>
        : complex_batch_traits<xtl::xcomplex<float, float, i3ec>, float, 8, 32>
    {
    };

    template <bool i3ec>
    class batch<xtl::xcomplex<float, float, i3ec>, 8>
        : public simd_complex_batch<batch<xtl::xcomplex<float, float, i3ec>, 8>>
    {
    public:

        using self_type = batch<xtl::xcomplex<float, float, i3ec>, 8>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = xtl::xcomplex<float, float, i3ec>;
        using real_batch = batch<float, 8>;

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

        friend class simd_complex_batch<batch<xtl::xcomplex<float, float, i3ec>, 8>>;
    };

    /******************************************************
     * batch_bool<xtl::xcomplex<double, double, i3ec>, 4> *
     ******************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch_bool<xtl::xcomplex<double, double, i3ec>, 4>>
        : complex_batch_bool_traits<xtl::xcomplex<double, double, i3ec>, double, 4, 32>
    {
    };

    template<bool i3ec>
    class batch_bool<xtl::xcomplex<double, double, i3ec>, 4>
        : public simd_complex_batch_bool<batch_bool<xtl::xcomplex<double, double, i3ec>, 4>>
    {
    public:

        using self_type = batch_bool<xtl::xcomplex<double, double, i3ec>, 4>;
        using base_type = simd_complex_batch_bool<self_type>;
        using real_batch = batch_bool<double, 4>;

        batch_bool() = default;
        using base_type::base_type;

        // VS2015 has a bug with inheriting constructors involving SFINAE
        batch_bool(bool b0, bool b1, bool b2, bool b3)
            : base_type(real_batch(b0, b1, b2, b3))
        {
        }
    };

    /*************************************************
     * batch<xtl::xcomplex<double, double, i3ec>, 4> *
     *************************************************/

    template <bool i3ec>
    struct simd_batch_traits<batch<xtl::xcomplex<double, double, i3ec>, 4>>
        : complex_batch_traits<xtl::xcomplex<double, double, i3ec>, double, 4, 32>
    {
    };

    template <bool i3ec>
    class batch<xtl::xcomplex<double, double, i3ec>, 4>
        : public simd_complex_batch<batch<xtl::xcomplex<double, double, i3ec>, 4>>
    {
    public:

        using self_type = batch<xtl::xcomplex<double, double, i3ec>, 4>;
        using base_type = simd_complex_batch<self_type>;
        using value_type = xtl::xcomplex<double, double, i3ec>;
        using real_batch = batch<double, 4>;

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

        friend class simd_complex_batch<batch<xtl::xcomplex<double>, 4>>;
    };

    /********************************************
     * batch<std::complex<T>, N> implementation *
     ********************************************/

    template <bool i3ec>
    inline batch<xtl::xcomplex<float, float, i3ec>, 8>&
    batch<xtl::xcomplex<float, float, i3ec>, 8>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        std::tie(this->m_real, this->m_imag) = detail::load_complex_f(hi, lo);
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 8>::get_complex_high() const -> real_batch
    {
        return detail::get_complex_high_f(this->m_real, this->m_imag);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<float, float, i3ec>, 8>::get_complex_low() const -> real_batch
    {
        return detail::get_complex_low_f(this->m_real, this->m_imag);
    }

    template <bool i3ec>
    inline batch<xtl::xcomplex<double, double, i3ec>, 4>&
    batch<xtl::xcomplex<double, double, i3ec>, 4>::load_complex(const real_batch& hi, const real_batch& lo)
    {
        std::tie(this->m_real, this->m_imag) = detail::load_complex_d(hi, lo);
        return *this;
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 4>::get_complex_high() const -> real_batch
    {
        return detail::get_complex_high_d(this->m_real, this->m_imag);
    }

    template <bool i3ec>
    inline auto batch<xtl::xcomplex<double, double, i3ec>, 4>::get_complex_low() const -> real_batch
    {
        return detail::get_complex_low_d(this->m_real, this->m_imag);
    }

#endif
}

#endif
