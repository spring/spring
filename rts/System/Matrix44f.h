#ifndef MATRIX44F_H
#define MATRIX44F_H

#include "Sync/Syncify.h"
#include "float3.h"

class CMatrix44f
{
public:
	CR_DECLARE_STRUCT(CMatrix44f);

	CMatrix44f();
	CMatrix44f(const float3& pos, const float3& x, const float3& y, const float3& z);
	explicit CMatrix44f(const float3& pos);

	CMatrix44f(const CMatrix44f& n);
	CMatrix44f& operator=(const CMatrix44f& n);

	void LoadIdentity();

	float& operator[](int a) { return m[a]; }
	float operator[](int a) const { return m[a]; }

	void RotateX(float rad);
	void RotateY(float rad);
	void RotateZ(float rad);
	void Rotate(float rad, const float3& axis);	//axis assumed normalized
	void Translate(float x, float y, float z);
	CMatrix44f Mul(const CMatrix44f& other) const;

	CMatrix44f& InvertInPlace();
	CMatrix44f Invert() const;

	float3 Mul(const float3& vect) const;
	float3 GetPos(void) const { return float3(m[12], m[13], m[14]); }

	void SetUpVector(float3& up);
	void Translate(const float3& pos);

	// OpenGL ordered (ie. column-major)
	float m[16];
};


// Templates for simple 2D/3D matrixes that behave
// pretty much like statically allocated matrixes,
// but can also be casted to and used as pointers.
template<class T>
T **newmat2(int x, int y) {
	T *mat2 = SAFE_NEW T[x*y], **mat = SAFE_NEW T *[x];
	for (int i = 0; i < x; ++i)
		mat[i] = mat2 + i*y;
	return mat;
}

template<class T>
T ***newmat3(int x, int y, int z) {
	T *mat3=SAFE_NEW T[x*y*z], **mat2=SAFE_NEW T *[x*y], ***mat=SAFE_NEW T **[x];
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
