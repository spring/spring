/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MATRIX44F_H
#define MATRIX44F_H

#include "float3.h"

class CMatrix44f
{
public:
	CR_DECLARE_STRUCT(CMatrix44f);

	CMatrix44f();

	template<typename T> CMatrix44f(const T* m44) {
		m[ 0] = float(m44[ 0]); m[ 1] = float(m44[ 1]); m[ 2] = float(m44[ 2]); m[ 3] = float(m44[ 3]);
		m[ 4] = float(m44[ 4]); m[ 5] = float(m44[ 5]); m[ 6] = float(m44[ 6]); m[ 7] = float(m44[ 7]);
		m[ 8] = float(m44[ 8]); m[ 9] = float(m44[ 9]); m[10] = float(m44[10]); m[11] = float(m44[11]);
		m[12] = float(m44[12]); m[13] = float(m44[13]); m[14] = float(m44[14]); m[15] = float(m44[15]);
	}

	CMatrix44f(const float3& pos, const float3& x, const float3& y, const float3& z);
	CMatrix44f(const float& rotX, const float& rotY, const float& rotZ);
	explicit CMatrix44f(const float3& pos);

	CMatrix44f(const CMatrix44f& n);
	CMatrix44f& operator=(const CMatrix44f& n);

	void LoadIdentity();

	float& operator[](int a) { return m[a]; }
	float operator[](int a) const { return m[a]; }

	void RotateX(float rad);
	void RotateY(float rad);
	void RotateZ(float rad);
	void Rotate(float rad, const float3& axis); // axis is assumed to be normalized
	void Translate(float x, float y, float z);
	CMatrix44f Mul(const CMatrix44f& other) const;

	CMatrix44f& InvertInPlace();
	CMatrix44f Invert() const;

	static double CalculateCofactor(const double m[4][4], int ei, int ej);
	static bool Invert(const double m[4][4], double mInv[4][4]);


	float3 Mul(const float3& vect) const;
	float3 GetPos(void) const { return float3(m[12], m[13], m[14]); }

	void SetUpVector(float3& up);
	void Translate(const float3& pos);

	/// OpenGL ordered (ie. column-major)
	float m[16];

	/// Allows implicit conversion to float* (for passing to gl functions)
	operator const float* () const { return m; }
	operator float* () { return m; }
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
