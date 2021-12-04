/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLOAT3_H
#define FLOAT3_H

#include <cassert>

#include "System/BranchPrediction.h"
#include "lib/streflop/streflop_cond.h"
#include "System/creg/creg_cond.h"
#include "System/FastMath.h"
#ifdef _MSC_VER
#include "System/Platform/Win/win32.h"
#endif


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
	CR_DECLARE_STRUCT(float3)


	/**
	 * @brief default Constructor
	 * With parameters, initializes x/y/z to 0.0f.
	 */
	constexpr float3() : x(0.0f), y(0.0f), z(0.0f) {}

	/**
	 * @brief Constructor
	 * @param x float x
	 * @param y float y
	 * @param z float z
	 *
	 * With parameters, initializes x/y/z to the given floats.
	 */
	constexpr float3(const float x, const float y, const float z)
			: x(x), y(y), z(z) {}

	/**
	 * @brief float[3] Constructor
	 * @param f float[3] to assign
	 *
	 * With parameters, initializes x/y/z to the given float[3].
	 */
	constexpr float3(const float f[3]) : x(f[0]), y(f[1]), z(f[2]) {}

	/**
	 * @brief operator =
	 * @param f float[3] to assign
	 *
	 * Sets the float3 to the given float[3].
	 */
	float3& operator= (const float f[3]) {
		x = f[0];
		y = f[1];
		z = f[2];
		return *this;
	}

	/**
	 * @brief Copy x, y, z into float[3]
	 * @param f float[3] to copy values into
	 *
	 * Sets the float[3] to this float3.
	 */
	void copyInto(float f[3]) const {
		f[0] = x;
		f[1] = y;
		f[2] = z;
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
	float3 operator+ (const float3& f) const {
		return float3(x + f.x, y + f.y, z + f.z);
	}

	/**
	 * @brief operator +
	 * @return sum of float3+float
	 * @param f single float to add
	 *
	 * When adding just a float, the point is
	 * increased in all directions by that float.
	 */
	float3 operator+ (const float f) const {
		return float3(x + f, y + f, z + f);
	}

	/**
	 * @brief operator +=
	 * @param f float3 reference to add.
	 *
	 * Just like adding a float3, but updates this
	 * float with the new sum.
	 */
	float3& operator+= (const float3& f) {
		x += f.x;
		y += f.y;
		z += f.z;
		return *this;
	}

	/**
	 * @brief operator -
	 * @param f float3 to subtract
	 * @return difference of float3s
	 *
	 * Decreases the float3 by another float3,
	 * subtracting each x/y/z component individually.
	 */
	float3 operator- (const float3& f) const {
		return float3(x - f.x, y - f.y, z - f.z);
	}

	/**
	 * @brief operator -
	 * @return difference of float3 and float
	 * @param f float to subtract
	 *
	 * When subtracting a single fixed float,
	 * decreases all three x/y/z components by that amount.
	 */
	float3 operator- (const float f) const {
		return float3(x - f, y - f, z - f);
	}

	/**
	 * @brief operator -=
	 * @param f float3 to subtract
	 *
	 * Same as subtracting a float3, but stores
	 * the new float3 inside this one.
	 */
	void operator-= (const float3& f) {
		x -= f.x;
		y -= f.y;
		z -= f.z;
	}


	/**
	 * @brief operator -
	 * @return inverted float3
	 *
	 * When negating the float3, inverts all three
	 * x/y/z components.
	 */
	float3 operator- () const {
		return float3(-x, -y, -z);
	}


	/**
	 * @brief operator *
	 * @param f float3 to multiply
	 * @return product of float3s
	 *
	 * When multiplying by another float3,
	 * multiplies each x/y/z component individually.
	 */
	float3 operator* (const float3& f) const {
		return float3(x * f.x, y * f.y, z * f.z);
	}

	/**
	 * @brief operator *
	 * @param f float to multiply
	 * @return product of float3 and float
	 *
	 * When multiplying by a single float, multiplies
	 * each x/y/z component by that float.
	 */
	float3 operator* (const float f) const {
		return float3(x * f, y * f, z * f);
	}

	/**
	 * @brief operator *=
	 * @param f float3 to multiply
	 *
	 * Same as multiplying a float3, but stores
	 * the new float3 inside this one.
	 */
	void operator*= (const float3& f) {
		x *= f.x;
		y *= f.y;
		z *= f.z;
	}

	/**
	 * @brief operator *=
	 * @param f float to multiply
	 *
	 * Same as multiplying a float, but stores
	 * the new float3 inside this one.
	 */
	float3& operator*= (const float f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	/**
	 * @brief operator /
	 * @param f float3 to divide
	 * @return divided float3
	 *
	 * When dividing by a float3, divides
	 * each x/y/z component individually.
	 */
	float3 operator/ (const float3& f) const {
		return float3(x / f.x, y / f.y, z / f.z);
	}

	/**
	 * @brief operator /
	 * @param f float to divide
	 * @return float3 divided by float
	 *
	 * When dividing by a single float, divides
	 * each x/y/z component by that float.
	 */
	float3 operator/ (const float f) const {
		return ((*this) * (1.0f / f));
	}

	/**
	 * @brief operator /=
	 * @param f float3 to divide
	 *
	 * Same as dividing by a float3, but stores
	 * the new values inside this float3.
	 */
	void operator/= (const float3& f) {
		x /= f.x;
		y /= f.y;
		z /= f.z;
	}

	/**
	 * @brief operator /=
	 * @param f float to divide
	 *
	 * Same as dividing by a single float, but stores
	 * the new values inside this float3.
	 */
	void operator/= (const float f) {
		(*this) *= (1.0f / f);
	}


	/**
	 * @brief operator ==
	 * @param f float3 to test
	 * @return whether float3s are equal under default cmp_eps tolerance in x/y/z
	 *
	 * Tests if this float3 is equal to another, by
	 * checking each x/y/z component individually.
	 */
	bool operator== (const float3& f) const {
		return (equals(f));
	}

	/**
	 * @brief operator !=
	 * @param f float3 to test
	 * @return whether float3s are not equal
	 *
	 * Tests if this float3 is not equal to another, by
	 * checking each x/y/z component individually.
	 */
	bool operator!= (const float3& f) const {
		return (!equals(f));
	}


	/**
	 * @brief operator[]
	 * @param t index in xyz array
	 * @return float component at index
	 *
	 * Array access for x/y/z components
	 * (index 0 is x, index 1 is y, index 2 is z)
	 */
	float& operator[] (const int t) {
		return (&x)[t];
	}

	/**
	 * @brief operator[] const
	 * @param t index in xyz array
	 * @return const float component at index
	 *
	 * Same as plain [] operator but used in
	 * a const context
	 */
	const float& operator[] (const int t) const {
		return (&x)[t];
	}

	/**
	 * @see operator==
	 */
	bool equals(const float3& f, const float3& eps = float3(cmp_eps(), cmp_eps(), cmp_eps())) const;


	/**
	 * @brief binary float3 equality
	 * @param f float3 to compare to
	 * @return const whether the two float3 are binary same
	 *
	 */
	bool same(const float3& f) const {
		return x == f.x && y == f.y && z == f.z;
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
	float dot(const float3& f) const {
		return (x * f.x) + (y * f.y) + (z * f.z);
	}

	/**
	 * @brief dot2D product
	 * @param f float3 to use
	 * @return 2D dot product of float3s
	 *
	 * Calculates the 2D dot product of this and
	 * another float3 (sums the products of
	 * x/z components).
	 */
	float dot2D(const float3& f) const {
		return (x * f.x) + (z * f.z);
	}

	/**
	 * @brief cross product
	 * @param f float3 to use
	 * @return cross product of two float3s
	 *
	 * Calculates the cross product of this and
	 * another float3:
	 * (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2)
	 */
	float3 cross(const float3& f) const {
		return float3(
				(y * f.z) - (z * f.y),
				(z * f.x) - (x * f.z),
				(x * f.y) - (y * f.x));
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
	float distance(const float3& f) const {
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return math::sqrt(dx*dx + dy*dy + dz*dz);
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
	float distance2D(const float3& f) const {
		const float dx = x - f.x;
		const float dz = z - f.z;
		return math::sqrt(dx*dx + dz*dz);
	}


	/**
	 * @brief SqDistance between float3s squared
	 * @param f float3 to compare against
	 * @return float squared distance between float3s
	 *
	 * Returns the squared distance of 2 float3s
	 */
	float SqDistance(const float3& f) const {
		const float dx = x - f.x;
		const float dy = y - f.y;
		const float dz = z - f.z;
		return (dx*dx + dy*dy + dz*dz);
	}

	/**
	 * @brief SqDistance2D between float3s (only x and z)
	 * @param f float3 to compare against
	 * @return 2D squared distance between float3s
	 *
	 * Returns the squared 2d-distance of 2 float3s
	 */
	float SqDistance2D(const float3& f) const {
		const float dx = x - f.x;
		const float dz = z - f.z;
		return (dx*dx + dz*dz);
	}


	/**
	 * @brief Length of this vector
	 * @return float length of vector
	 *
	 * Returns the length of this vector
	 * (squares and sums each x/y/z component,
	 * square root for pythagorean theorem)
	 */
	float Length() const {
		//assert(x!=0.f || y!=0.f || z!=0.f);
		return math::sqrt(SqLength());
	}

	/**
	 * @brief 2-dimensional length of this vector
	 * @return 2D float length of vector
	 *
	 * Returns the 2-dimensional length of this vector
	 * (squares and sums only the x and z components,
	 * square root for pythagorean theorem)
	 */
	float Length2D() const {
		//assert(x!=0.f || y!=0.f || z!=0.f);
		return math::sqrt(SqLength2D());
	}

	/**
	 * @brief length squared
	 * @return length squared
	 *
	 * Returns the length of this vector squared.
	 */
	float SqLength() const {
		return (x*x + y*y + z*z);
	}

	/**
	 * @brief 2-dimensional length squared
	 * @return 2D length squared
	 *
	 * Returns the 2-dimensional length of this
	 * vector squared.
	 */
	float SqLength2D() const {
		return (x*x + z*z);
	}

	/**
	 * normalize vector in-place, return its old length
	 */
	float LengthNormalize() {
		const float len = Length();

		if (likely(len > nrm_eps()))
			(*this) *= (1.0f / len);

		return len;
	}

	float LengthNormalize2D() {
		y = 0.0f; return LengthNormalize();
	}


	/**
	 * @brief normalizes the vector using one of Normalize implementations
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	float3& Normalize() {
#if defined(__SUPPORT_SNAN__)
#ifndef BUILDING_AI
		return SafeNormalize();
#endif
		return UnsafeNormalize();
#else
		return SafeNormalize();
#endif
	}

	float3& Normalize2D() {
		y = 0.0f; return Normalize();
	}


	/**
	 * @brief normalizes the vector without checking for zero vector
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	float3& UnsafeNormalize() {
		return ((*this) *= math::isqrt(SqLength()));
	}

	float3& UnsafeNormalize2D() {
		y = 0.0f; return UnsafeNormalize();
	}


	/**
	 * @brief normalizes the vector safely (check for *this == ZeroVector)
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each
	 * x/y/z component by the vector's length.
	 */
	float3& SafeNormalize() {
		const float sql = SqLength();

		if (likely(sql > nrm_eps()))
			(*this) *= math::isqrt(sql);

		return *this;
	}

	float3& SafeNormalize2D() {
		y = 0.0f; return SafeNormalize();
	}


	/**
	 * @brief normalizes the vector approximately
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each x/y/z component by
	 * the vector's approx. length.
	 */
	float3& ANormalize() {
#if defined(__SUPPORT_SNAN__)
#ifndef BUILDING_AI
		return SafeANormalize();
#endif
		return UnsafeANormalize();
#else
		return SafeANormalize();
#endif
	}

	float3& ANormalize2D() {
		y = 0.0f; return ANormalize();
	}


	/**
	 * @brief normalizes the vector approximately without checking
	 *        for ZeroVector
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each x/y/z component by
	 * the vector's approx. length.
	 */
	float3& UnsafeANormalize() {
		assert(SqLength() > nrm_eps());
		return ((*this) *= math::isqrt(SqLength()));
	}

	float3& UnsafeANormalize2D() {
		y = 0.0f; return UnsafeANormalize();
	}


	/**
	 * @brief normalizes the vector approximately and safely
	 * @return pointer to self
	 *
	 * Normalizes the vector by dividing each x/y/z component by
	 * the vector's approximate length, if (this != ZeroVector),
	 * else do nothing.
	 */
	float3& SafeANormalize() {
		const float sql = SqLength();

		if (likely(sql > nrm_eps()))
			(*this) *= math::isqrt(sql);

		return *this;
	}

	float3& SafeANormalize2D() {
		y = 0.0f; return SafeANormalize();
	}


	static bool CheckNaN(float c) { return (!math::isnan(c) && !math::isinf(c)); }

	bool CheckNaNs() const { return (CheckNaN(x) && CheckNaN(y) && CheckNaN(z)); }
	void AssertNaNs() const {
		assert(CheckNaN(x));
		assert(CheckNaN(y));
		assert(CheckNaN(z));
	}


	/**
	 * @brief Check against FaceHeightmap bounds
	 *
	 * Check if this vector is in bounds [0 .. mapDims.mapxy-1]
	 * @note THIS IS THE WRONG SPACE! _ALL_ WORLD SPACE POSITIONS SHOULD BE IN VertexHeightmap RESOLUTION!
	 * @see #IsInMap
	 */
	bool IsInBounds() const;
	/**
	 * @brief Check against FaceHeightmap bounds
	 *
	 * Check if this vector is in map [0 .. mapDims.mapxy]
	 * @note USE THIS!
	 */
	bool IsInMap() const;


	/**
	 * @brief Clamps to FaceHeightmap
	 *
	 * Clamps to the `face heightmap` resolution [0 .. mapDims.mapxy-1] * SQUARE_SIZE
	 * @note THIS IS THE WRONG SPACE! _ALL_ WORLD SPACE POSITIONS SHOULD BE IN VertexHeightmap RESOLUTION!
	 * @deprecated  use ClampInMap instead, but see the note!
	 * @see #ClampInMap
	 */
	void ClampInBounds();

	/**
	 * @brief Clamps to VertexHeightmap
	 *
	 * Clamps to the `vertex heightmap`/`opengl space` resolution [0 .. mapDims.mapxy] * SQUARE_SIZE
	 * @note USE THIS!
	 */
	void ClampInMap();

	float3 cClampInBounds() const { float3 f = *this; f.ClampInBounds(); return f; }
	float3 cClampInMap() const { float3 f = *this; f.ClampInMap(); return f; }

	static float3 min(const float3 v1, const float3 v2);
	static float3 max(const float3 v1, const float3 v2);
	static float3 fabs(const float3 v);
	static float3 sign(const float3 v);

	static constexpr float cmp_eps() { return 1e-04f; }
	static constexpr float nrm_eps() { return 1e-12f; }

	/**
	 * @brief max x pos
	 *
	 * Static value containing the maximum x position (:= mapDims.mapx-1)
	 * @note maxxpos is set after loading the map.
	 */
	static float maxxpos;

	/**
	 * @brief max z pos
	 *
	 * Static value containing the maximum z position (:= mapDims.mapy-1)
	 * @note maxzpos is set after loading the map.
	 */
	static float maxzpos;


public:
	union {
		struct { float x,y,z; };
		struct { float r,g,b; };
		struct { float x1,y1,x2; };
		struct { float s,t,p; };
		struct { float xstart, ystart, xend; };
		struct { float xyz[3]; };
	};
};

/**
 * @brief upwards vector
 *
 * Defines constant upwards vector
 * (0, 1, 0)
 */
static constexpr float3  UpVector(0.0f, 1.0f, 0.0f);
static constexpr float3 FwdVector(0.0f, 0.0f, 1.0f);
static constexpr float3 RgtVector(1.0f, 0.0f, 0.0f);

/**
 * @brief zero vector
 *
 * Defines constant zero vector
 * (0, 0, 0)
 */
static constexpr float3 ZeroVector(0.0f, 0.0f, 0.0f);
static constexpr float3 OnesVector(1.0f, 1.0f, 1.0f);

static constexpr float3 XYVector(1.0f, 1.0f, 0.0f);
static constexpr float3 XZVector(1.0f, 0.0f, 1.0f);
static constexpr float3 YZVector(0.0f, 1.0f, 1.0f);

#endif /* FLOAT3_H */

