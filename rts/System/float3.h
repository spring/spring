/**
 * @file float3.h
 * @brief float3 header
 *
 * Declaration of float3 class
 */
#ifndef FLOAT3_H
#define FLOAT3_H

#include "lib/streflop/streflop_cond.h"
#include "creg/creg_cond.h"
#include "ExternalAI/Interface/SAIFloat3.h"
#include "FastMath.h"

/**
 * @brief float3 class
 *
 * Contains a set of 3 float numbers.
 * Usually used to represent a vector in
 * space as x/y/z.
 */
class float3
{
public:
	CR_DECLARE_STRUCT(float3);
/*	inline void* operator new(size_t size){return mempool.Alloc(size);};
	inline void* operator new(size_t n, void *p){return p;}; //cp visual
	inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
*/
	/**
	 * @brief Constructor
	 *
	 * With no parameters, x/y/z are just initialized to 0.
	 */
	inline float3() : x(0), y(0), z(0) {};

	/**
	 * @brief Constructor
	 * @param x float x
	 * @param y float y
	 * @param z float z
	 *
	 * With parameters, initializes x/y/z to the given floats.
	 */
	inline float3(const float x,const float y,const float z) : x(x),y(y),z(z) {};

	/**
	 * @brief Constructor
	 * @param sAIFloat3 SAIFLoat3 sAIFLoat3
	 *
	 * With parameters, initializes x/y/z to the given SAIFLoat3.
	 */
	inline float3(const SAIFloat3& sAIFloat3) : x(sAIFloat3.x), y(sAIFloat3.y), z(sAIFloat3.z) {};

	/**
	 * @brief operator =
	 * @param t index in xyz array
	 * @return const float3
	 *
	 * Same as plain [] operator but used in
	 * a const context
	 */
	inline float3& operator= (const SAIFloat3& sAIFloat3) {
		
		x = sAIFloat3.x;
		y = sAIFloat3.y;
		z = sAIFloat3.z;

		return *this;
	}

	/**
	 * @brief Destructor
	 *
	 * Does nothing
	 */
	inline ~float3(){};


	/**
	 * @brief operator =
	 * @param f float[3] to assign
	 *
	 * Sets the float3 to the given float[3].
	 */
	inline void operator= (const float f[3]){
		x=f[0];
		y=f[1];
		z=f[2];
	}


	/**
	 * @brief operator +
	 * @param f float3 reference to add.
	 * @return sum of float3s
	 *
	 * When adding another float3, will
	 * calculate the sum of the positions in
	 * space (adds the x/y/z components individually)
	 */
	inline float3 operator+ (const float3 &f) const{
		return float3(x+f.x,y+f.y,z+f.z);
	}

	/**
	 * @brief operator +
	 * @return sum of float3+float
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
	 * @param f float3 reference to add.
	 *
	 * Just like adding a float3, but updates this
	 * float with the new sum.
	 */
	inline void operator+= (const float3 &f){
		x+=f.x;
		y+=f.y;
		z+=f.z;
	}

	/**
	 * @brief operator -
	 * @param f float3 to subtract
	 * @return difference of float3s
	 *
	 * Decreases the float3 by another float3,
	 * subtracting each x/y/z component individually.
	 */
	inline float3 operator- (const float3 &f) const{
		return float3(x-f.x,y-f.y,z-f.z);
	}

	/**
	 * @brief operator -
	 * @return inverted float3
	 *
	 * When negating the float3, inverts all three
	 * x/y/z components.
	 */
	inline float3 operator- () const{
		return float3(-x,-y,-z);
	}

	/**
	 * @brief operator -
	 * @return difference of float3 and float
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
	 * @param f float3 to subtract
	 *
	 * Same as subtracting a float3, but stores
	 * the new float3 inside this one.
	 */
	inline void operator-= (const float3 &f){
		x-=f.x;
		y-=f.y;
		z-=f.z;
	}

	/**
	 * @brief operator *
	 * @param f float3 to multiply
	 * @return product of float3s
	 *
	 * When multiplying by another float3,
	 * multiplies each x/y/z component individually.
	 */
	inline float3 operator* (const float3 &f) const{
		return float3(x*f.x,y*f.y,z*f.z);
	}

	/**
	 * @brief operator *
	 * @param f float to multiply
	 * @return product of float3 and float
	 *
	 * When multiplying by a single float, multiplies
	 * each x/y/z component by that float.
	 */
	inline float3 operator* (const float f) const{
		return float3(x*f,y*f,z*f);
	}

	/**
	 * @brief operator *=
	 * @param f float3 to multiply
	 *
	 * Same as multiplying a float3, but stores
	 * the new float3 inside this one.
	 */
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
	 * the new float3 inside this one.
	 */
	inline void operator*= (const float f){
		x*=f;
		y*=f;
		z*=f;
	}

	/**
	 * @brief operator /
	 * @param f float3 to divide
	 * @return divided float3
	 *
	 * When dividing by a float3, divides
	 * each x/y/z component individually.
	 */
	inline float3 operator/ (const float3 &f) const{
		return float3(x/f.x,y/f.y,z/f.z);
	}

	/**
	 * @brief operator /
	 * @param f float to divide
	 * @return float3 divided by float
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
	 * @param f float3 to divide
	 *
	 * Same as dividing by a float3, but stores
	 * the new values inside this float3.
	 */
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
	 * the new values inside this float3.
	 */
	inline void operator/= (const float f){
		const float inv = (float) 1.f / f;
		x *= inv;
		y *= inv;
		z *= inv;
	}

	/**
	 * @brief operator ==
	 * @param f float3 to test
	 * @return whether float3s are equal
	 *
	 * Tests if this float3 is equal to another, by
	 * checking each x/y/z component individually.
	 */
	inline bool operator== (const float3 &f) const {
		return math::fabs(x-f.x) <= math::fabs(1.0E-4f*x) && math::fabs(y-f.y) <= math::fabs(1.0E-4f*y) && math::fabs(z-f.z) <= math::fabs(1.0E-4f*z);
	}

	/**
	 * @brief operator !=
	 * @param f float3 to test
	 * @return whether float3s are not equal
	 *
	 * Tests if this float3 is not equal to another, by
	 * checking each x/y/z component individually.
	 */
	inline bool operator!= (const float3 &f) const {
		return !(*this == f);
	}

	/**
	 * @brief operator[]
	 * @param t index in xyz array
	 * @return float component at index
	 *
	 * Array access for x/y/z components
	 * (index 0 is x, index 1 is y, index 2 is z)
	 */
	inline float& operator[] (const int t) {
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
	inline const float& operator[] (const int t) const {
		return (&x)[t];
	}

	/**
	 * @brief dot product
	 * @param f float3 to use
	 * @return dot product of float3s
	 *
	 * Calculates the dot product of this and
	 * another float3 (sums the products of each
	 * x/y/z component).
	 */
	inline float dot (const float3 &f) const{
		return x*f.x + y*f.y + z*f.z;
	}

	/**
	 * @brief cross product
	 * @param f float3 to use
	 * @return cross product of two float3s
	 *
	 * Calculates the cross product of this and
	 * another float3 (y1*z2-z1*y2,z1*x2-x1*z2,x1*y2-y1*x2)
	 */
	inline float3 cross(const float3 &f) const{
		return float3(	y*f.z - z*f.y,
						z*f.x - x*f.z,
						x*f.y - y*f.x  );
	}

	/**
	 * @brief distance between float3s
	 * @param f float3 to compare against
	 * @return float distance between float3s
	 *
	 * Calculates the distance between this float3
	 * and another float3 (sums the differences in each
	 * x/y/z component, square root for pythagorean theorem)
	 */
	inline float distance(const float3 &f) const{
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (float) math::sqrt(dx*dx + dy*dy + dz*dz);
	}

	/**
	 * @brief distance2D between float3s (only x and z)
	 * @param f float3 to compare against
	 * @return 2D distance between float3s
	 *
	 * Calculates the distance between this float3
	 * and another float3 2-dimensionally (that is,
	 * only using the x and z components).  Sums the
	 * differences in the x and z components, square
	 * root for pythagorean theorem
	 */
	inline float distance2D(const float3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float) math::sqrt(dx*dx + dz*dz);
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
		return (float) math::sqrt(x*x+y*y+z*z);
	}

	/**
	 * @brief 2-dimensional length of this vector
	 * @return 2D float length of vector
	 *
	 * Returns the 2-dimensional length of this vector
	 * (squares and sums only the x and z components,
	 * square root for pythagorean theorem)
	 */
	inline float Length2D() const {
		return (float) math::sqrt(x * x + z * z);
	}

	/**
	 * @brief normalizes the vector approximately
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's approx.
	 * length.
	 *
	 * Measured compile time hit: statistically insignificant (1%)
	 */
	inline float3& ANormalize() {
		float invL = fastmath::isqrt(SqLength());
		if (invL != 0.f) {
			x *= invL;
			y *= invL;
			z *= invL;
		}
		return *this;
	}

	/**
	 * @brief normalizes the vector
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	inline float3& Normalize() {
		const float L = math::sqrt(x * x + y * y + z * z);
		if (L != 0.f) {
			const float invL = (float) 1.f / L;
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
		return x*x + y*y + z*z;
	}

	/**
	 * @brief 2-dimensional length squared
	 * @return 2D length squared
	 *
	 * Returns the 2-dimensional length of this
	 * vector squared.
	 */
	inline float SqLength2D() const{
		return x*x + z*z;
	}


	/**
	 * @brief SqDistance between float3s squared
	 * @param f float3 to compare against
	 * @return float squared distance between float3s
	 *
	 * Returns the squared distance of 2 float3s
	 */
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
	inline float SqDistance2D(const float3 &f) const{
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (float)(dx*dx + dz*dz);
	}


	/**
	 * @brief max x pos
	 *
	 * Static value containing the maximum
	 * x position
	 * @note maxxpos is set after loading the map.
	 */
	static float maxxpos;

	/**
	 * @brief max z pos
	 *
	 * Static value containing the maximum
	 * z position
	 * @note maxzpos is set after loading the map.
	 */
	static float maxzpos;

	bool IsInBounds() const; //!< Check if this vector is in bounds without clamping x and z
	bool CheckInBounds(); //!< Check if this vector is in bounds and clamp x and z if not
	
	SAIFloat3 toSAIFloat3() const;

	float x; ///< x component
	float y; ///< y component
	float z; ///< z component
};

/**
 * @brief upwards vector
 *
 * Defines constant upwards vector
 * (0,1,0)
 */
const float3 UpVector(0,1,0);

/**
 * @brief zero vector
 *
 * Defines constant zero vector
 * (0,0,0)
 */
const float3 ZeroVector(0,0,0);


#endif /* FLOAT3_H */
