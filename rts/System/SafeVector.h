/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SAFE_VECTOR_H
#define _SAFE_VECTOR_H

#include <vector>

#define USE_SAFE_VECTOR

#ifdef USE_SAFE_VECTOR
#include "System/creg/creg_cond.h"

template<class T>
class safe_vector : public std::vector<T>
{
	CR_DECLARE_STRUCT(safe_vector);

public:
	typedef typename std::vector<T>::size_type size_type;

	safe_vector(): showError(true) {}
	safe_vector(size_type size, T value): std::vector<T>(size, value), showError(true) {}
	safe_vector(const safe_vector<T>& vec): std::vector<T>(vec), showError(true) {}

	const T& operator[] (const size_type i) const {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::operator[](i);
	}
	T& operator[] (const size_type i) {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::operator[](i);
	}

	const T& at (const size_type i) const {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::at(i);
	}
	T& at (const size_type i) {
		if (i >= std::vector<T>::size())
			return safe_element(i);
		return std::vector<T>::at(i);
	}

private:
	const T& safe_element(size_type idx) const;
	      T& safe_element(size_type idx);

	mutable bool showError;
};

#else
#define safe_vector std::vector
#endif

#endif // _SAFE_VECTOR_H
