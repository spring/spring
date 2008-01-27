#ifndef SFLOAT3_H_
#define SFLOAT3_H_

/**
 * @brief Simple 3d vector of floats
 * 
 * Doesn't use streflop in contrast to float3
 */
class SFloat3
{
public:
	/**
	 * @brief Constructor
	 *
	 * With no parameters, x/y/z are just initialized to 0.
	 */
	inline SFloat3() : x(0), y(0), z(0) {};
	
	/**
	 * @brief Constructor
	 * @param x float x
	 * @param y float y
	 * @param z float z
	 *
	 * With parameters, initializes x/y/z to the given floats.
	 */
	inline SFloat3(const float x,const float y,const float z) : x(x),y(y),z(z) {};
	
	float x; ///< x component
	float y; ///< y component
	float z; ///< z component
};

#endif /*SFLOAT3_H_*/
