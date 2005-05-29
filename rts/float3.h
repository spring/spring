#ifndef FLOAT3_H
#define FLOAT3_H
// float3.h: interface for the float3 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FLOAT3_H__026451C1_6B7D_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_FLOAT3_H__026451C1_6B7D_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include <math.h>

class float3
{
public:
/*	inline void* operator new(size_t size){return mempool.Alloc(size);};
	inline void* operator new(size_t n, void *p){return p;}; //cp visual
	inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
*/
	inline float3() : x(0), y(0), z(0) {};

	inline float3(const float x,const float y,const float z) : x(x),y(y),z(z) {}

	inline ~float3(){};

	inline float3 operator+ (const float3 &f) const{
		return float3(x+f.x,y+f.y,z+f.z);
	}
	inline float3 operator+ (const float f) const{
		return float3(x+f,y+f,z+f);
	}
	inline void operator+= (const float3 &f){
		x+=f.x;
		y+=f.y;
		z+=f.z;
	}

	inline float3 operator- (const float3 &f) const{
		return float3(x-f.x,y-f.y,z-f.z);
	}
	inline float3 operator- () const{
		return float3(-x,-y,-z);
	}
	inline float3 operator- (const float f) const{
		return float3(x-f,y-f,z-f);
	}
	inline void operator-= (const float3 &f){
		x-=f.x;
		y-=f.y;
		z-=f.z;
	}

	inline float3 operator* (const float3 &f) const{
		return float3(x*f.x,y*f.y,z*f.z);
	}
	inline float3 operator* (const float f) const{
		return float3(x*f,y*f,z*f);
	}
	inline void operator*= (const float3 &f){
		x*=f.x;
		y*=f.y;
		z*=f.z;
	}
	inline void operator*= (const float f){
		x*=f;
		y*=f;
		z*=f;
	}

	inline float3 operator/ (const float3 &f) const{
		return float3(x/f.x,y/f.y,z/f.z);
	}
	inline float3 operator/ (const float f) const{
		return float3(x/f,y/f,z/f);
	}
	inline void operator/= (const float3 &f){
		x/=f.x;
		y/=f.y;
		z/=f.z;
	}
	inline void operator/= (const float f){
		x/=f;
		y/=f;
		z/=f;
	}

	inline bool operator== (const float3 &f) const {
		return ((x==f.x) && (y==f.y) && (z==f.z));
	}
	inline bool operator!= (const float3 &f) const {
		return ((x!=f.x) || (y!=f.y) || (z!=f.z));
	}
	inline float& operator[] (const int t) {
		return xyz[t];
	}
	inline const float& operator[] (const int t) const {
		return xyz[t];
	}

	inline float dot (const float3 &f) const{
		return x*f.x + y*f.y + z*f.z;
	}
	inline float3 cross(const float3 &f) const{
		float3 ut;
		ut.x=y*f.z-z*f.y;
		ut.y=z*f.x-x*f.z;
		ut.z=x*f.y-y*f.x;
		return ut;
	}
	inline float distance(const float3 &f) const{
		return (float)sqrt((x-f.x)*(x-f.x)+(y-f.y)*(y-f.y)+(z-f.z)*(z-f.z));
	}
	inline float distance2D(const float3 &f) const{
		return (float)sqrt((x-f.x)*(x-f.x)+(z-f.z)*(z-f.z));
	}
	inline float Length() const{
		return (float)sqrt(x*x+y*y+z*z);
	}
	inline float Length2D() const{
		return (float)sqrt(x*x+z*z);
	}
	inline float3& Normalize()
	{
		float l=(float)sqrt(x*x+y*y+z*z);
		if(l!=0){
			x=x/l;
			y=y/l;
			z=z/l;
		}
		return *this;
	}
	inline float SqLength() const{
		return x*x+y*y+z*z;
	}
	inline float SqLength2D() const{
		return x*x+z*z;
	}
	union {
		struct{
			float x;
			float y;
			float z;
		};
		float xyz[3];
	};

	static float maxxpos;
	static float maxzpos;

	bool CheckInBounds();
};

#endif // !defined(AFX_FLOAT3_H__026451C1_6B7D_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* FLOAT3_H */
