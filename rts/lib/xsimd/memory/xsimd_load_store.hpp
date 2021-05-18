/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_LOAD_STORE_HPP
#define XSIMD_LOAD_STORE_HPP

#include "../config/xsimd_config.hpp"
#include "../types/xsimd_traits.hpp"

namespace xsimd
{
    /******************************
     * Data transfer instructions *
     ******************************/

    /**
     * @defgroup data_transfer Data Transfer Instructions
     */

    /**
     * @ingroup data_transfer
     * Returns a batch with all values initialized to \c value.
     * @param value the scalar used to initialize the batch.
     * @return the batch wrapping the highest available instruction set.
     */
    template <class T1, class T2 = T1>
    simd_return_type<T1, T2> set_simd(const T1& value);

    /**
     * @ingroup data_transfer
     * Loads the memory array pointed to by \c src into a batch and returns it.
     * \c src is required to be aligned.
     * @param src the pointer to the memory array to load.
     * @return the batch wrapping the highest available instruction set.
     */
    template <class T1, class T2 = T1>
    simd_return_type<T1, T2> load_aligned(const T1* src);

    /**
     * @ingroup data_transfer
     * Loads the memory array pointed to by \c src into the batch \c dst.
     * \c src is required to be aligned.
     * @param src the pointer to the memory array to load.
     * @param dst the destination batch.
     */
    template <class T1, class T2 = T1>
    void load_aligned(const T1* src, simd_type<T2>& dst);

    /**
     * @ingroup data_transfer
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into a batch of complex numbers and returns it. \c real_src and
     * \c imag_src are required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @return the batch of complex wrapping the highest available instruction set.
     */
    template <class T1, class T2>
    simd_return_type<T1, T2> load_aligned(const T1* real_src, const T1* imag_src);

    /**
     * @ingroup data_transfer
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into the batch \c dst. \c real_src and \c imag_src are required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @param dst the destination batch.
     */
    template <class T1, class T2>
    void load_aligned(const T1* real_src, const T1* imag_src, simd_type<T2>& dst);

    /**
     * @ingroup data_transfer
     * Loads the memory array pointed to by \c src into a batch and returns it.
     * \c src is not required to be aligned.
     * @param src the pointer to the memory array to load.
     * @return the batch wrapping the highest available instruction set.
     */
    template <class T1, class T2 = T1>
    simd_return_type<T1, T2> load_unaligned(const T1* src);

    /**
     * @ingroup data_transfer
     * Loads the memory array pointed to by \c src into the batch \c dst.
     * \c src is not required to be aligned.
     * @param src the pointer to the memory array to load.
     * @param dst the destination batch.
     */
    template <class T1, class T2 = T1>
    void load_unaligned(const T1* src, simd_type<T2>& dst);

    /**
     * @ingroup data_transfer
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into a batch of complex numbers and returns it. \c real_src and
     * \c imag_src are not required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @return the batch of complex wrapping the highest available instruction set.
     */
    template <class T1, class T2>
    simd_return_type<T1, T2> load_unaligned(const T1* real_src, const T1* imag_src);

    /**
     * @ingroup data_transfer
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into the batch \c dst. \c real_src and \c imag_src are not required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @param dst the destination batch.
     */
    template <class T1, class T2>
    void load_unaligned(const T1* real_src, const T1* imag_src, simd_type<T2>& dst);

    /**
     * @ingroup data_transfer
     * Stores the batch \c src into  the memory array pointed to by \c dst.
     * \c dst is required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the batch to store.
     */
    template <class T1, class T2 = T1>
    void store_aligned(T1* dst, const simd_type<T2>& src);

    /**
     * @ingroup data_transfer
     * Stores the boolean batch \c src into  the memory array pointed to by \c dst.
     * \c dst is required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the boolean batch to store.
     */
    template <class T1, class T2 = T1>
    void store_aligned(T1* dst, const simd_bool_type<T2>& src);

    /**
     * @ingroup data_transfer
     * Stores the batch \c src into  the memory array pointed to by \c dst.
     * \c dst is not required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the batch to store.
     */
    template <class T1, class T2 = T1>
    void store_unaligned(T1* dst, const simd_type<T2>& src);

    /**
     * @ingroup data_transfer
     * Stores the boolean batch \c src into  the memory array pointed to by \c dst.
     * \c dst is not required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the boolean batch to store.
     */
    template <class T1, class T2 = T1>
    void store_unaligned(T1* dst, const simd_bool_type<T2>& src);

    /**
     * @ingroup data_transfer
     * Stores the batch of complex numbers \c src into  the memory arrays pointed
     * to by \c real_dst and \c imag_dst. \c real_dst and \c imag_dst are required
     * to be aligned.
     * @param real_dst the pointer to the memory array of the real part.
     * @param imag_dst the pointer to the memory array of the imaginary part.
     * @param src the batch to store.
     */
    template <class T1, class T2>
    void store_aligned(T1* real_dst, T1* imag_dst, const simd_type<T2>& src);

    /**
     * @ingroup data_transfer
     * Stores the batch of complex numbers \c src into  the memory arrays pointed
     * to by \c real_dst and \c imag_dst. \c real_dst and \c imag_dst are not required
     * to be aligned.
     * @param real_dst the pointer to the memory array of the real part.
     * @param imag_dst the pointer to the memory array of the imaginary part.
     * @param src the batch to store.
     */
    template <class T1, class T2>
    void store_unaligned(T1* real_dst, T1* imag_dst, const simd_type<T2>& src);

    // Load / store generic functions

    /**
     * @defgroup generic_load_store Generic load and store
     */

     /**
      * @ingroup generic_load_store
      * Loads the memory array pointed to by \c src into a batch and returns it.
      * \c src is required to be aligned.
      * @param src the pointer to the memory array to load.
      * @return the batch wrapping the highest available instruction set.
      */
    template <class T1, class T2 = T1>
    simd_return_type<T1, T2> load_simd(const T1* src, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory array pointed to by \c src into the batch \c dst.
     * \c src is required to be aligned.
     * @param src the pointer to the memory array to load.
     * @param dst the destination batch.
     */
    template <class T1, class T2 = T1>
    void load_simd(const T1* src, simd_type<T2>& dst, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into a batch of complex numbers and returns it. \c real_src and
     * \c imag_src are required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @return the batch of complex wrapping the highest available instruction set.
     */
    template <class T1, class T2>
    simd_return_type<T1, T2> load_simd(const T1* real_src, const T1* imag_src, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into the batch \c dst. \c real_src and \c imag_src are required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @param dst the destination batch.
     */
    template <class T1, class T2>
    void load_simd(const T1* real_src, const T1* imag_src, simd_type<T2>& dst, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory array pointed to by \c src into a batch and returns it.
     * \c src is not required to be aligned.
     * @param src the pointer to the memory array to load.
     * @return the batch wrapping the highest available instruction set.
     */
    template <class T1, class T2 = T1>
    simd_return_type<T1, T2> load_simd(const T1* src, unaligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory array pointed to by \c src into the batch \c dst.
     * \c src is not required to be aligned.
     * @param src the pointer to the memory array to load.
     * @param dst the destination batch.
     */
    template <class T1, class T2 = T1>
    void load_simd(const T1* src, simd_type<T2>& dst, unaligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into a batch of complex numbers and returns it. \c real_src and
     * \c imag_src are not required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @return the batch of complex wrapping the highest available instruction set.
     */
    template <class T1, class T2>
    simd_return_type<T1, T2> load_simd(const T1* real_src, const T1* imag_src, unaligned_mode);

    /**
     * @ingroup generic_load_store
     * Loads the memory arrays pointed to by \c real_src and \c imag_src
     * into the batch \c dst. \c real_src and \c imag_src are not required to be aligned.
     * @param real_src the pointer to the memory array containing the real part.
     * @param imag_src the pointer to the memory array containing the imaginary part.
     * @param dst the destination batch.
     */
    template <class T1, class T2>
    void load_simd(const T1* real_src, const T1* imag_src, simd_type<T2>& dst, unaligned_mode);

    /**
     * @ingroup generic_load_store
     * Stores the batch \c src into  the memory array pointed to by \c dst.
     * \c dst is required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the batch to store.
     */
    template <class T1, class T2 = T1>
    void store_simd(T1* dst, const simd_type<T2>& src, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Stores the boolean batch \c src into  the memory array pointed to by \c dst.
     * \c dst is required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the boolean batch to store.
     */
    template <class T1, class T2 = T1>
    void store_simd(T1* dst, const simd_bool_type<T2>& src, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Stores the batch \c src into  the memory array pointed to by \c dst.
     * \c dst is not required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the batch to store.
     */
    template <class T1, class T2 = T1>
    void store_simd(T1* dst, const simd_type<T2>& src, unaligned_mode);

    /**
     * @ingroup generic_load_store
     * Stores the boolean batch \c src into  the memory array pointed to by \c dst.
     * \c dst is not required to be aligned.
     * @param dst the pointer to the memory array.
     * @param src the boolean batch to store.
     */
    template <class T1, class T2 = T1>
    void store_simd(T1* dst, const simd_bool_type<T2>& src, unaligned_mode);

    /**
     * @ingroup generic_load_store
     * Stores the batch of complex numbers \c src into  the memory arrays pointed
     * to by \c real_dst and \c imag_dst. \c real_dst and \c imag_dst are required
     * to be aligned.
     * @param real_dst the pointer to the memory array of the real part.
     * @param imag_dst the pointer to the memory array of the imaginary part.
     * @param src the batch to store.
     */
    template <class T1, class T2>
    void store_simd(T1* real_dst, T1* imag_dst, const simd_type<T2>& src, aligned_mode);

    /**
     * @ingroup generic_load_store
     * Stores the batch of complex numbers \c src into  the memory arrays pointed
     * to by \c real_dst and \c imag_dst. \c real_dst and \c imag_dst are not required
     * to be aligned.
     * @param real_dst the pointer to the memory array of the real part.
     * @param imag_dst the pointer to the memory array of the imaginary part.
     * @param src the batch to store.
     */
    template <class T1, class T2>
    void store_simd(T1* real_dst, T1* imag_dst, const simd_type<T2>& src, unaligned_mode);

    // Prefetch

    template <class T>
    void prefetch(const T* address);


    /***************************
     * detail implementation
     ***************************/

    namespace detail
    {
        // Common implementation of SIMD functions for types supported
        // by vectorization.
        template <class T, class V>
        struct simd_function_invoker
        {
            inline static V set_simd(const T& value)
            {
                using batch_value_type = typename V::value_type;
                using value_type = typename std::conditional<std::is_same<T, bool>::value,
                                                             bool,
                                                             batch_value_type>::type;
                return V(value_type(value));
            }

            inline static V load_aligned(const T* src)
            {
                V res;
                return res.load_aligned(src);
            }

            inline static void load_aligned(const T* src, V& dst)
            {
                dst.load_aligned(src);
            }

            inline static V load_unaligned(const T* src)
            {
                V res;
                return res.load_unaligned(src);
            }

            inline static void load_unaligned(const T* src, V& dst)
            {
                dst.load_unaligned(src);
            }

            inline static void store_aligned(T* dst, const V& src)
            {
                src.store_aligned(dst);
            }

            inline static void store_unaligned(T* dst, const V& src)
            {
                src.store_unaligned(dst);
            }
        };

        template <class T, class V>
        struct simd_complex_invoker
        {
            inline static V load_aligned(const T* real_src, const T* imag_src)
            {
                V res;
                return res.load_aligned(real_src, imag_src);
            }

            inline static void load_aligned(const T* real_src, const T* imag_src, V& dst)
            {
                dst.load_aligned(real_src, imag_src);
            }

            inline static V load_unaligned(const T* real_src, const T* imag_src)
            {
                V res;
                return res.load_unaligned(real_src, imag_src);
            }

            inline static void load_unaligned(const T* real_src, const T* imag_src, V& dst)
            {
                dst.load_unaligned(real_src, imag_src);
            }

            inline static void store_aligned(T* real_dst, T* imag_dst, const V& src)
            {
                src.store_aligned(real_dst, imag_dst);
            }

            inline static void store_unaligned(T* real_dst, T* imag_dst, const V& src)
            {
                src.store_unaligned(real_dst, imag_dst);
            }
        };

        // Default implementation of SIMD functions for types not supported
        // by vectorization.
        template <class T>
        struct simd_function_invoker<T, T>
        {
            inline static T set_simd(const T& value)
            {
                return value;
            }

            inline static T load_aligned(const T* src)
            {
                return *src;
            }

            inline static void load_aligned(const T* src, T& dst)
            {
                dst = *src;
            }

            inline static T load_unaligned(const T* src)
            {
                return *src;
            }

            inline static void load_unaligned(const T* src, T& dst)
            {
                dst = *src;
            }

            inline static void store_aligned(T* dst, const T& src)
            {
                *dst = src;
            }

            inline static void store_unaligned(T* dst, const T& src)
            {
                *dst = src;
            }
        };
    }

    /***********************************************
     * Data transfer instructions implementation
     ***********************************************/

    template <class T1, class T2>
    inline simd_return_type<T1, T2> set_simd(const T1& value)
    {
        return detail::simd_function_invoker<T1, simd_return_type<T1, T2>>::set_simd(value);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_aligned(const T1* src)
    {
        return detail::simd_function_invoker<T1, simd_return_type<T1, T2>>::load_aligned(src);
    }

    template <class T1, class T2>
    inline void load_aligned(const T1* src, simd_type<T2>& dst)
    {
        detail::simd_function_invoker<T1, simd_type<T2>>::load_aligned(src, dst);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_aligned(const T1* real_src, const T1* imag_src)
    {
        return detail::simd_complex_invoker<T1, simd_return_type<T1, T2>>::load_aligned(real_src, imag_src);
    }

    template <class T1, class T2>
    inline void load_aligned(const T1* real_src, const T1* imag_src, simd_type<T2>& dst)
    {
        detail::simd_complex_invoker<T1, simd_type<T2>>::load_aligned(real_src, imag_src, dst);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_unaligned(const T1* src)
    {
        return detail::simd_function_invoker<T1, simd_return_type<T1, T2>>::load_unaligned(src);
    }

    template <class T1, class T2>
    inline void load_unaligned(const T1* src, simd_type<T2>& dst)
    {
        detail::simd_function_invoker<T1, simd_type<T2>>::load_unaligned(src, dst);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_unaligned(const T1* real_src, const T1* imag_src)
    {
        return detail::simd_complex_invoker<T1, simd_return_type<T1, T2>>::load_unaligned(real_src, imag_src);
    }

    template <class T1, class T2>
    inline void load_unaligned(const T1* real_src, const T1* imag_src, simd_type<T2>& dst)
    {
        detail::simd_complex_invoker<T1, simd_type<T2>>::load_unaligned(real_src, imag_src, dst);
    }

    template <class T1, class T2>
    inline void store_aligned(T1* dst, const simd_type<T2>& src)
    {
        detail::simd_function_invoker<T1, simd_type<T2>>::store_aligned(dst, src);
    }

    template <class T1, class T2>
    inline void store_aligned(T1* dst, const simd_bool_type<T2>& src)
    {
        detail::simd_function_invoker<T1, simd_bool_type<T2>>::store_aligned(dst, src);
    }

    template <class T1, class T2>
    inline void store_unaligned(T1* dst, const simd_type<T2>& src)
    {
        detail::simd_function_invoker<T1, simd_type<T2>>::store_unaligned(dst, src);
    }

    template <class T1, class T2>
    inline void store_unaligned(T1* dst, const simd_bool_type<T2>& src)
    {
        detail::simd_function_invoker<T1, simd_bool_type<T2>>::store_unaligned(dst, src);
    }

    template <class T1, class T2>
    inline void store_aligned(T1* real_dst, T1* imag_dst, const simd_type<T2>& src)
    {
        detail::simd_complex_invoker<T1, simd_type<T2>>::store_aligned(real_dst, imag_dst, src);
    }

    template <class T1, class T2>
    inline void store_unaligned(T1* real_dst, T1* imag_dst, const simd_type<T2>& src)
    {
        detail::simd_complex_invoker<T1, simd_type<T2>>::store_unaligned(real_dst, imag_dst, src);
    }

    /***************************************************
     * Load / store generic functions implementation
     ***************************************************/

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_simd(const T1* src, aligned_mode)
    {
        return load_aligned<T1, T2>(src);
    }

    template <class T1, class T2>
    inline void load_simd(const T1* src, simd_type<T2>& dst, aligned_mode)
    {
        load_aligned<T1, T2>(src, dst);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_simd(const T1* real_src, const T1* imag_src, aligned_mode)
    {
        return load_aligned<T1, T2>(real_src, imag_src);
    }

    template <class T1, class T2>
    inline void load_simd(const T1* real_src, const T1* imag_src, simd_type<T2>& dst, aligned_mode)
    {
        load_aligned<T1, T2>(real_src, imag_src, dst);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_simd(const T1* src, unaligned_mode)
    {
        return load_unaligned<T1, T2>(src);
    }

    template <class T1, class T2>
    inline void load_simd(const T1* src, simd_type<T2>& dst, unaligned_mode)
    {
        load_unaligned<T1, T2>(src, dst);
    }

    template <class T1, class T2>
    inline simd_return_type<T1, T2> load_simd(const T1* real_src, const T1* imag_src, unaligned_mode)
    {
        return load_unaligned<T1, T2>(real_src, imag_src);
    }

    template <class T1, class T2>
    inline void load_simd(const T1* real_src, const T1* imag_src, simd_type<T2>& dst, unaligned_mode)
    {
        load_unaligned<T1, T2>(real_src, imag_src, dst);
    }

    template <class T1, class T2>
    inline void store_simd(T1* dst, const simd_type<T2>& src, aligned_mode)
    {
        store_aligned<T1, T2>(dst, src);
    }

    template <class T1, class T2>
    inline void store_simd(T1* dst, const simd_bool_type<T2>& src, aligned_mode)
    {
        store_aligned<T1, T2>(dst, src);
    }

    template <class T1, class T2>
    inline void store_simd(T1* dst, const simd_type<T2>& src, unaligned_mode)
    {
        store_unaligned<T1, T2>(dst, src);
    }

    template <class T1, class T2>
    inline void store_simd(T1* dst, const simd_bool_type<T2>& src, unaligned_mode)
    {
        store_unaligned<T1, T2>(dst, src);
    }

    template <class T1, class T2>
    inline void store_simd(T1* real_dst, T1* imag_dst, const simd_type<T2>& src, aligned_mode)
    {
        store_aligned<T1, T2>(real_dst, imag_dst, src);
    }

    template <class T1, class T2>
    inline void store_simd(T1* real_dst, T1* imag_dst, const simd_type<T2>& src, unaligned_mode)
    {
        store_unaligned<T1, T2>(real_dst, imag_dst, src);
    }

    /*****************************
     * Prefetch implementation
     *****************************/

    template <class T>
    inline void prefetch(const T* /*address*/)
    {
    }

#if XSIMD_X86_INSTR_SET > XSIMD_VERSION_NUMBER_NOT_AVAILABLE

    template <>
    inline void prefetch<int32_t>(const int32_t* address)
    {
        _mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_T0);
    }

    template <>
    inline void prefetch<int64_t>(const int64_t* address)
    {
        _mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_T0);
    }

    template <>
    inline void prefetch<float>(const float* address)
    {
        _mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_T0);
    }

    template <>
    inline void prefetch<double>(const double* address)
    {
        _mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_T0);
    }

#endif

}

#endif
