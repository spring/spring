#ifndef FLOAT3_H
#define FLOAT3_H
// float3.h: interface for the float3 class.
//
//////////////////////////////////////////////////////////////////////

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
		const float inv = (float) 1. / f;
		return float3(x*inv, y*inv, z*inv);
	}
	inline void operator/= (const float3 &f){
		x/=f.x;
		y/=f.y;
		z/=f.z;
	}
	inline void operator/= (const float f){
		const float inv = (float) 1. / f;
		x *= inv;
		y *= inv;
		z *= inv;
	}

	inline bool operator== (const float3 &f) const {
		return !(fabs(x-f.x) > 1.0E-34 || fabs(y-f.y) > 1.0E-34 || fabs(z-f.z) > 1.0E-34) ;
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
		return float3(	y*f.z - z*f.y,
						z*f.x - x*f.z,
						x*f.y - y*f.x  );
	}
	inline float distance(const float3 &f) const{
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (float) sqrt(dx*dx + dy*dy + dz*dz);
	}
	inline float distance2D(const float3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float)sqrt(dx*dx + dz*dz);
	}
	inline float Length() const{
		return (float)sqrt(x*x+y*y+z*z);
	}
	inline float Length2D() const{
		return (float)sqrt(x*x+z*z);
	}
	inline float3& Normalize()
	{
		const float L = sqrt(x*x + y*y + z*z);
		if(L != 0.){
			const float invL = (float) 1. / L;
			x *= invL;
			y *= invL;
			z *= invL;
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

#endif /* FLOAT3_H */
