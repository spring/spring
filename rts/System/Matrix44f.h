/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MATRIX44F_H
#define MATRIX44F_H

#include "System/float3.h"

class CMatrix44f
{
public:
	CR_DECLARE_STRUCT(CMatrix44f);

	CMatrix44f();
	CMatrix44f(const CMatrix44f& mat);

	CMatrix44f(const float3& pos, const float3& x, const float3& y, const float3& z);
	CMatrix44f(const float& rotX, const float& rotY, const float& rotZ);
	explicit CMatrix44f(const float3& pos);

	void LoadIdentity();

	float& operator[](int a) { return m[a]; }
	float operator[](int a) const { return m[a]; }

	void SetUpVector(const float3& up);
	void RotateX(float rad);
	void RotateY(float rad);
	void RotateZ(float rad);
	void Rotate(float rad, const float3& axis); //! axis is assumed to be normalized
	void Scale(const float3 scales);
	void Translate(float x, float y, float z);
	void Translate(const float3& pos);
	void SetPos(float x, float y, float z);
	void SetPos(const float3& pos);
	float3 GetPos() const { return float3(m[12], m[13], m[14]); }

	float3 Mul(const float3& vect) const;

	void Transpose();

	CMatrix44f operator* (const CMatrix44f& other) const;
	CMatrix44f& operator*= (const CMatrix44f& other);
	inline void operator*= (const float a) {
		for (size_t i=0; i < 16; ++i)
			m[i] *= a;
	}

	//! affine matrix inversion
	CMatrix44f& InvertAffineInPlace();
	CMatrix44f InvertAffine() const;

	//! general matrix inversion
	bool InvertInPlace();
	CMatrix44f Invert(bool* status = NULL) const;

	/// Allows implicit conversion to float* (for passing to gl functions)
	operator const float* () const { return m; }
	operator float* () { return m; }

public:
	/// OpenGL ordered (ie. column-major)
	union {
		float m[16];
		float md[4][4]; //! WARNING: it still is column-major, means md[j][i]!!!
	};
};


// Templates for simple 2D/3D matrixes that behave
// pretty much like statically allocated matrixes,
// but can also be casted to and used as pointers.
template<class T>
T **newmat2(int x, int y) {
	T *mat2 = new T[x*y], **mat = new T *[x];
	for (int i = 0; i < x; ++i)
		mat[i] = mat2 + i*y;
	return mat;
}

template<class T>
T ***newmat3(int x, int y, int z) {
	T *mat3=new T[x*y*z], **mat2=new T *[x*y], ***mat=new T **[x];
	for (int i = 0; i < x; ++i) {
		for(int j = 0; j < y; ++j)
			mat2[i*y+j] = mat3 + (i*y+j)*z;
		mat[i] = mat2 + i*y;
	}
	return mat;
}

template<class T>
void delmat2(T** mat) {
	delete [] *mat;
	delete [] mat;
}

template<class T>
void delmat3(T*** mat) {
	delete [] **mat;
	delete [] *mat;
	delete [] mat;
}

#endif /* MATRIX44F_H */
