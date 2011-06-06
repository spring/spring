/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __SAFE_VECTOR_H__
#define __SAFE_VECTOR_H__

#include <vector>

#define USE_SAFE_VECTOR

#ifdef USE_SAFE_VECTOR
#include "System/creg/creg_cond.h"

template<class T>
class safe_vector : public std::vector<T> {
private:
	CR_DECLARE_STRUCT(safe_vector);

	mutable bool showError;

	const T& safe_element(size_t idx) const;
	      T& safe_element(size_t idx);

public:
	safe_vector(): showError(true) {}
	safe_vector(size_t size, T value): std::vector<T>(size, value), showError(true) {}
	safe_vector(const safe_vector<T>& vec): std::vector<T>(vec), showError(true) {}

	const T& operator[] (const typename std::vector<T>::size_type i) const {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::operator[](i);
	}
	T& operator[] (const typename std::vector<T>::size_type i) {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::operator[](i);
	}

	const T& at (const typename std::vector<T>::size_type i) const {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::at(i);
	}
	T& at (const typename std::vector<T>::size_type i) {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::at(i);
	}
};

#else
#define safe_vector std::vector
#endif

#endif
