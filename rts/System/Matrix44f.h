/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MATRIX44F_H
#define MATRIX44F_H

#include "System/float3.h"
#include "System/float4.h"

class CMatrix44f
{
public:
	CR_DECLARE_STRUCT(CMatrix44f)

	CMatrix44f();
	CMatrix44f(const CMatrix44f& mat);

	CMatrix44f(const float3 pos, const float3 x, const float3 y, const float3 z);
	CMatrix44f(const float rotX, const float rotY, const float rotZ);
	explicit CMatrix44f(const float3 pos);

	bool IsOrthoNormal() const;
	bool IsIdentity() const;

	CMatrix44f& LoadIdentity();

	void SetUpVector(const float3 up);
	CMatrix44f& RotateX(float angle); // (pitch) angle in radians
	CMatrix44f& RotateY(float angle); // (  yaw) angle in radians
	CMatrix44f& RotateZ(float angle); // ( roll) angle in radians
	CMatrix44f& Rotate(float angle, const float3 axis); // assumes axis is normalized
	CMatrix44f& RotateEulerXYZ(const float3 angles); // executes Rotate{X,Y,Z}
	CMatrix44f& RotateEulerYXZ(const float3 angles); // executes Rotate{Y,X,Z}
	CMatrix44f& RotateEulerZXY(const float3 angles); // executes Rotate{Z,X,Y}
	CMatrix44f& RotateEulerZYX(const float3 angles); // executes Rotate{Z,Y,X}
	CMatrix44f& Translate(const float x, const float y, const float z);
	CMatrix44f& Translate(const float3 pos) { return Translate(pos.x, pos.y, pos.z); }
	CMatrix44f& Scale(const float3 scales);

	void SetPos(const float3 pos) { m[12] = pos.x; m[13] = pos.y; m[14] = pos.z; }
	void SetX  (const float3 dir) { m[ 0] = dir.x; m[ 1] = dir.y; m[ 2] = dir.z; }
	void SetY  (const float3 dir) { m[ 4] = dir.x; m[ 5] = dir.y; m[ 6] = dir.z; }
	void SetZ  (const float3 dir) { m[ 8] = dir.x; m[ 9] = dir.y; m[10] = dir.z; }

	float3& GetPos() { return col[3]; }
	const float3& GetPos() const { return col[3]; }
	const float3& GetX()   const { return col[0]; }
	const float3& GetY()   const { return col[1]; }
	const float3& GetZ()   const { return col[2]; }

	float3 GetEulerAnglesLftHand(float eps = 0.01f /*std::numeric_limits<float>::epsilon()*/) const;
	float3 GetEulerAnglesRgtHand(float eps = 0.01f /*std::numeric_limits<float>::epsilon()*/) const;

	inline void operator *= (const float a) {
		for (size_t i = 0; i < 16; i += 4) {
			m[i + 0] *= a;
			m[i + 1] *= a;
			m[i + 2] *= a;
			m[i + 3] *= a;
		}
	}

	CMatrix44f& Transpose();

	/// general matrix inversion
	bool InvertInPlace();
	CMatrix44f Invert(bool* status = NULL) const;

	/// affine matrix inversion
	CMatrix44f& InvertAffineInPlace();
	CMatrix44f  InvertAffine() const;

	/// vector multiply
	float3 operator* (const float3 v) const;
	float3 Mul(const float3 v) const { return ((*this) * v); }
	float4 operator* (const float4 v) const;

	/// matrix multiply
	CMatrix44f  operator  *  (const CMatrix44f& mat) const;
	CMatrix44f& operator >>= (const CMatrix44f& mat);
	CMatrix44f& operator <<= (const CMatrix44f& mat);
	CMatrix44f& operator  *= (const CMatrix44f& mat) { return ((*this) <<= mat); }

	// matrix addition
	CMatrix44f& operator += (const CMatrix44f& mat) { return ((*this) = (*this) + mat); }
	CMatrix44f  operator +  (const CMatrix44f& mat) const {
		CMatrix44f r;

		for (size_t i = 0; i < 16; i += 4) {
			r[i + 0] = m[i + 0] + mat[i + 0];
			r[i + 1] = m[i + 1] + mat[i + 1];
			r[i + 2] = m[i + 2] + mat[i + 2];
			r[i + 3] = m[i + 3] + mat[i + 3];
		}

		return r;
	}

	float& operator [] (int a)       { return m[a]; }
	float  operator [] (int a) const { return m[a]; }

	/// Allows implicit conversion to float* (for passing to gl functions)
	operator const float* () const { return m; }
	operator       float* ()       { return m; }

	enum {
		ANGLE_P = 0,
		ANGLE_Y = 1,
		ANGLE_R = 2,
	};

public:
	/// OpenGL ordered (ie. column-major)
	union {
		float m[16];
		float md[4][4]; // WARNING: it still is column-major, means md[j][i]!!!
		float4 col[4];
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
