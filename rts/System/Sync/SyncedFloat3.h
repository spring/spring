/**
 * @file SyncedFloat3.h
 * @brief SyncedFloat3 header
 *
 * Declaration of SyncedFloat3 class
 */
#ifndef SYNCEDFLOAT3_H
#define SYNCEDFLOAT3_H

#include "float3.h"

#if defined(SYNCDEBUG) || defined(SYNCCHECK)

#include "lib/streflop/streflop_cond.h"
#include "SyncedPrimitive.h"

/**
 * @brief SyncedFloat3 class
 *
 * Contains a set of 3 float numbers.
 * Usually used to represent a vector in
 * space as x/y/z.
 */
class SyncedFloat3
{
public:
	CR_DECLARE(SyncedFloat3);
/*	inline void* operator new(size_t size){return mempool.Alloc(size);};
	inline void* operator new(size_t n, void *p){return p;}; //cp visual
	inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
*/
	/**
	 * @brief Constructor
	 *
	 * With no parameters, x/y/z are just initialized to 0.
	 */
	inline SyncedFloat3() : x(0), y(0), z(0) {};

	/**
	 * @brief Constructor
	 * @param x float x
	 * @param y float y
	 * @param z float z
	 *
	 * With parameters, initializes x/y/z to the given floats.
	 */
	inline SyncedFloat3(const float x,const float y,const float z) : x(x),y(y),z(z) {}

	/**
	 * @brief Copy constructor
	 * @param f SyncedFloat3 to copy
	 */
	inline SyncedFloat3(const SyncedFloat3& f) : x(f.x), y(f.y), z(f.z) {}
	inline SyncedFloat3(const float3& f) : x(f.x), y(f.y), z(f.z) {}

	/**
	 * @brief Destructor
	 *
	 * Does nothing
	 */
	inline ~SyncedFloat3(){}

	/**
	 * @brief operator +
	 * @param f SyncedFloat3 reference to add.
	 * @return sum of float3s
	 *
	 * When adding another SyncedFloat3, will
	 * calculate the sum of the positions in
	 * space (adds the x/y/z components individually)
	 */
	inline float3 operator+ (const SyncedFloat3 &f) const{
		return float3(x+f.x,y+f.y,z+f.z);
	}
	inline float3 operator+ (const float3 &f) const{
		return float3(x+f.x,y+f.y,z+f.z);
	}
	inline friend float3 operator+ (const float3 &f, const SyncedFloat3 &g) {
		return float3(f.x+g.x,f.y+g.y,f.z+g.z);
	}

	/**
	 * @brief operator +
	 * @return sum of SyncedFloat3+float
	 * @param f single float to add
	 *
	 * When adding just a float, the point is
	 * increased in all directions by that float.
	 */
	inline float3 operator+ (const float f) const{
		return float3(x+f,y+f,z+f);
	}

	/**
	 * @brief operator +=
	 * @param SyncedFloat3
	 *
	 * Just like adding a SyncedFloat3, but updates this
	 * float with the new sum.
	 */
	inline void operator+= (const SyncedFloat3 &f){
		x+=f.x;
		y+=f.y;
		z+=f.z;
	}
	inline void operator+= (const float3 &f){
		x+=f.x;
		y+=f.y;
		z+=f.z;
	}

	/**
	 * @brief operator -
	 * @param f SyncedFloat3 to subtract
	 * @return difference of float3s
	 *
	 * Decreases the SyncedFloat3 by another SyncedFloat3,
	 * subtracting each x/y/z component individually.
	 */
	inline float3 operator- (const SyncedFloat3 &f) const{
		return float3(x-f.x,y-f.y,z-f.z);
	}
	inline float3 operator- (const float3 &f) const{
		return float3(x-f.x,y-f.y,z-f.z);
	}
	inline friend float3 operator- (const float3 &f, const SyncedFloat3 &g) {
		return float3(f.x-g.x,f.y-g.y,f.z-g.z);
	}

	/**
	 * @brief operator -
	 * @return inverted SyncedFloat3
	 *
	 * When negating the SyncedFloat3, inverts all three
	 * x/y/z components.
	 */
	inline float3 operator- () const{
		return float3(-x,-y,-z);
	}

	/**
	 * @brief operator -
	 * @return difference of SyncedFloat3 and float
	 * @param f float to subtract
	 *
	 * When subtracting a single fixed float,
	 * decreases all three x/y/z components by that amount.
	 */
	inline float3 operator- (const float f) const{
		return float3(x-f,y-f,z-f);
	}

	/**
	 * @brief operator -=
	 * @param f SyncedFloat3 to subtract
	 *
	 * Same as subtracting a SyncedFloat3, but stores
	 * the new SyncedFloat3 inside this one.
	 */
	inline void operator-= (const SyncedFloat3 &f){
		x-=f.x;
		y-=f.y;
		z-=f.z;
	}
	inline void operator-= (const float3 &f){
		x-=f.x;
		y-=f.y;
		z-=f.z;
	}

	/**
	 * @brief operator *
	 * @param f SyncedFloat3 to multiply
	 * @return product of float3s
	 *
	 * When multiplying by another SyncedFloat3,
	 * multiplies each x/y/z component individually.
	 */
	inline float3 operator* (const SyncedFloat3 &f) const{
		return float3(x*f.x,y*f.y,z*f.z);
	}
	inline float3 operator* (const float3 &f) const{
		return float3(x*f.x,y*f.y,z*f.z);
	}
	inline friend float3 operator* (const float3 &f, const SyncedFloat3 &g) {
		return float3(f.x*g.x,f.y*g.y,f.z*g.z);
	}

	/**
	 * @brief operator *
	 * @param f float to multiply
	 * @return product of SyncedFloat3 and float
	 *
	 * When multiplying by a single float, multiplies
	 * each x/y/z component by that float.
	 */
	inline float3 operator* (const float f) const{
		return float3(x*f,y*f,z*f);
	}

	/**
	 * @brief operator *=
	 * @param f SyncedFloat3 to multiply
	 *
	 * Same as multiplying a SyncedFloat3, but stores
	 * the new SyncedFloat3 inside this one.
	 */
	inline void operator*= (const SyncedFloat3 &f){
		x*=f.x;
		y*=f.y;
		z*=f.z;
	}
	inline void operator*= (const float3 &f){
		x*=f.x;
		y*=f.y;
		z*=f.z;
	}

	/**
	 * @brief operator *=
	 * @param f float to multiply
	 *
	 * Same as multiplying a float, but stores
	 * the new SyncedFloat3 inside this one.
	 */
	inline void operator*= (const float f){
		x*=f;
		y*=f;
		z*=f;
	}

	/**
	 * @brief operator /
	 * @param f SyncedFloat3 to divide
	 * @return divided SyncedFloat3
	 *
	 * When dividing by a SyncedFloat3, divides
	 * each x/y/z component individually.
	 */
	inline float3 operator/ (const SyncedFloat3 &f) const{
		return float3(x/f.x,y/f.y,z/f.z);
	}
	inline float3 operator/ (const float3 &f) const{
		return float3(x/f.x,y/f.y,z/f.z);
	}
	inline friend float3 operator/ (const float3 &f, const SyncedFloat3 &g) {
		return float3(f.x/g.x,f.y/g.y,f.z/g.z);
	}

	/**
	 * @brief operator /
	 * @param f float to divide
	 * @return SyncedFloat3 divided by float
	 *
	 * When dividing by a single float, divides
	 * each x/y/z component by that float.
	 */
	inline float3 operator/ (const float f) const{
		const float inv = (float) 1.f / f;
		return float3(x*inv, y*inv, z*inv);
	}

	/**
	 * @brief operator /=
	 * @param f SyncedFloat3 to divide
	 *
	 * Same as dividing by a SyncedFloat3, but stores
	 * the new values inside this SyncedFloat3.
	 */
	inline void operator/= (const SyncedFloat3 &f){
		x/=f.x;
		y/=f.y;
		z/=f.z;
	}
	inline void operator/= (const float3 &f){
		x/=f.x;
		y/=f.y;
		z/=f.z;
	}

	/**
	 * @brief operator /=
	 * @param f float to divide
	 *
	 * Same as dividing by a single float, but stores
	 * the new values inside this SyncedFloat3.
	 */
	inline void operator/= (const float f){
		const float inv = (float) 1.f / f;
		x *= inv;
		y *= inv;
		z *= inv;
	}

	/**
	 * @brief operator ==
	 * @param f SyncedFloat3 to test
	 * @return whether float3s are equal
	 *
	 * Tests if this SyncedFloat3 is equal to another, by
	 * checking each x/y/z component individually.
	 */
	inline bool operator== (const SyncedFloat3 &f) const {
		return fabs(x-f.x) <= fabs(1.0E-4f*x) && fabs(y-f.y) <= fabs(1.0E-4f*y) && fabs(z-f.z) <= fabs(1.0E-4f*z);
	}
	inline bool operator== (const float3 &f) const {
		return fabs(x-f.x) <= fabs(1.0E-4f*x) && fabs(y-f.y) <= fabs(1.0E-4f*y) && fabs(z-f.z) <= fabs(1.0E-4f*z);
	}
	inline friend bool operator== (const float3 &f, const SyncedFloat3 &g)  {
		return g == f;
	}

	/**
	 * @brief operator !=
	 * @param f SyncedFloat3 to test
	 * @return whether float3s are not equal
	 *
	 * Tests if this SyncedFloat3 is not equal to another, by
	 * checking each x/y/z component individually.
	 */
	inline bool operator!= (const SyncedFloat3 &f) const {
		return !(*this == f);
	}
	inline bool operator!= (const float3 &f) const {
		return !(*this == f);
	}
	inline friend bool operator!= (const float3 &f, const SyncedFloat3 &g)  {
		return !(g == f);
	}

	/**
	 * @brief operator[]
	 * @param t index in xyz array
	 * @return float component at index
	 *
	 * Array access for x/y/z components
	 * (index 0 is x, index 1 is y, index 2 is z)
	 */
	inline SyncedFloat& operator[] (const int t) {
		return (&x)[t];
	}

	/**
	 * @brief operator [] const
	 * @param t index in xyz array
	 * @return const float component at index
	 *
	 * Same as plain [] operator but used in
	 * a const context
	 */
	inline float operator[] (const int t) const {
		return (&x)[t];
	}

	/**
	 * @brief dot product
	 * @param f SyncedFloat3 to use
	 * @return dot product of float3s
	 *
	 * Calculates the dot product of this and
	 * another SyncedFloat3 (sums the products of each
	 * x/y/z component).
	 */
	inline float dot (const SyncedFloat3 &f) const{
		return x*f.x + y*f.y + z*f.z;
	}
	inline float dot (const float3 &f) const{
		return x*f.x + y*f.y + z*f.z;
	}

	/**
	 * @brief cross product
	 * @param f SyncedFloat3 to use
	 * @return cross product of two float3s
	 *
	 * Calculates the cross product of this and
	 * another SyncedFloat3 (y1*z2-z1*y2,z1*x2-x1*z2,x1*y2-y1*x2)
	 */
	inline float3 cross(const SyncedFloat3 &f) const{
		return float3(	y*f.z - z*f.y,
						z*f.x - x*f.z,
						x*f.y - y*f.x  );
	}
	inline float3 cross(const float3 &f) const{
		return float3(	y*f.z - z*f.y,
						z*f.x - x*f.z,
						x*f.y - y*f.x  );
	}

	/**
	 * @brief distance between float3s
	 * @param f SyncedFloat3 to compare against
	 * @return float distance between float3s
	 *
	 * Calculates the distance between this SyncedFloat3
	 * and another SyncedFloat3 (sums the differences in each
	 * x/y/z component, square root for pythagorean theorem)
	 */
	inline float distance(const SyncedFloat3 &f) const{
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (float) sqrt(dx*dx + dy*dy + dz*dz);
	}
	inline float distance(const float3 &f) const{
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (float) sqrt(dx*dx + dy*dy + dz*dz);
	}

	/**
	 * @brief distance2D between float3s (only x and y)
	 * @param f SyncedFloat3 to compare against
	 * @return 2D distance between float3s
	 *
	 * Calculates the distance between this SyncedFloat3
	 * and another SyncedFloat3 2-dimensionally (that is,
	 * only using the x and y components).  Sums the
	 * differences in the x and y components, square
	 * root for pythagorean theorem
	 */
	inline float distance2D(const SyncedFloat3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float) sqrt(dx*dx + dz*dz);
	}
	inline float distance2D(const float3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float) sqrt(dx*dx + dz*dz);
	}

	/**
	 * @brief Length of this vector
	 * @return float length of vector
	 *
	 * Returns the length of this vector
	 * (squares and sums each x/y/z component,
	 * square root for pythagorean theorem)
	 */
	inline float Length() const{
		return (float) sqrt(x*x+y*y+z*z);
	}

	/**
	 * @brief 2-dimensional length of this vector
	 * @return 2D float length of vector
	 *
	 * Returns the 2-dimensional length of this vector
	 * (squares and sums only the x and z components,
	 * square root for pythagorean theorem)
	 */
	inline float Length2D() const{
		return (float) sqrt(x*x+z*z);
	}

	/**
	 * @brief normalizes the vector
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	inline SyncedFloat3& Normalize()
	{
		// contrary to most other operations we can make this synced
		// because the results are always written in the synced x,y,z components
		const SyncedFloat L = sqrt(x*x + y*y + z*z);
		if(L != 0.f){
			const SyncedFloat invL = (float) 1.f / L;
			x *= invL;
			y *= invL;
			z *= invL;
		}
		return *this;
	}

	/**
	 * @brief length squared
	 * @return length squared
	 *
	 * Returns the length of this vector squared.
	 */
	inline float SqLength() const{
		return x*x+y*y+z*z;
	}

	/**
	 * @brief 2-dimensional length squared
	 * @return 2D length squared
	 *
	 * Returns the 2-dimensional length of this
	 * vector squared.
	 */
	inline float SqLength2D() const{
		return x*x+z*z;
	}


	/**
	 * @brief SqDistance between float3s squared
	 * @param f float3 to compare against
	 * @return float squared distance between float3s
	 *
	 * Returns the squared distance of 2 float3s
	 */
	inline float SqDistance(const SyncedFloat3 &f) const{
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (float) (dx*dx + dy*dy + dz*dz);
	}
	inline float SqDistance(const float3 &f) const{
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (float) (dx*dx + dy*dy + dz*dz);
	}


	/**
	 * @brief SqDistance2D between float3s (only x and z)
	 * @param f float3 to compare against
	 * @return 2D squared distance between float3s
	 *
	 * Returns the squared 2d-distance of 2 float3s
	 */
	inline float SqDistance2D(const SyncedFloat3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float) (dx*dx + dz*dz);
	}
	inline float SqDistance2D(const float3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float) (dx*dx + dz*dz);
	}


	SyncedFloat x; ///< x component
	SyncedFloat y; ///< y component
	SyncedFloat z; ///< z component

	bool CheckInBounds(); //!< Check if this vector is in bounds

	/**
	 * @brief cast operator
	 *
	 * @return a float3 with the same x/y/z components as this SyncedFloat3
	 */
	operator float3() const { return float3(x, y, z); }
};

#else // SYNCDEBUG || SYNCCHECK

typedef float3 SyncedFloat3;

#endif // !SYNCDEBUG && !SYNCCHECK

// this macro looks like a noop, but causes checksum update
#ifdef SYNCDEBUG
#define ASSERT_SYNCED_FLOAT3(x) { SyncedFloat3(x); }
#else
#define ASSERT_SYNCED_FLOAT3(x)
#endif

#endif // SYNCEDFLOAT3_H
