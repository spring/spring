#ifndef MATRIX44F_H
#define MATRIX44F_H

#include "float3.h"

class CMatrix44f
{
public:
	CMatrix44f(void);
	CMatrix44f(const float3& pos,const float3& x,const float3& y,const float3& z);
	~CMatrix44f(void);

	inline float& operator[](int a){return m[a];};
	inline float operator[](int a)const {return m[a];};
	void operator=(const CMatrix44f& other){for(int a=0;a<16;++a) m[a]=other[a];};

	void RotateX(float rad);
	void RotateY(float rad);
	void RotateZ(float rad);
	void Rotate(float rad, float3& axis);	//axis assumed normalized
	void Translate(float x, float y, float z);
	CMatrix44f Mul(const CMatrix44f& other) const;
	float3 Mul(const float3& vect) const;
	inline float3 GetPos(void){ return float3(m[12],m[13],m[14]); }

	float m[16];		//opengl ordered
	void SetUpVector(float3& up);
	void Translate(const float3& pos);
};

#endif /* MATRIX44F_H */
