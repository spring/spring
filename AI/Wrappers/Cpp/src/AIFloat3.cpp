/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIFloat3.h"

#include <string>

springai::AIFloat3::AIFloat3()
	: float3(0.0f, 0.0f, 0.0f)
{
}
springai::AIFloat3::AIFloat3(float x, float y, float z)
	: float3(x, y, z)
{
}
springai::AIFloat3::AIFloat3(float* xyz)
	: float3(xyz)
{
}
springai::AIFloat3::AIFloat3(const springai::AIFloat3& other)
	: float3(other)
{
}
springai::AIFloat3::AIFloat3(const float3& f3)
	: float3(f3)
{
}



void springai::AIFloat3::LoadInto(float* xyz) const {
	xyz[0] = x;
	xyz[1] = y;
	xyz[2] = z;
}

std::string springai::AIFloat3::ToString() const {

	char resBuff[128];
	SNPRINTF(resBuff, sizeof(resBuff), "(%f, %f, %f)", x, y, z);
	return std::string(resBuff);
}


/*int springai::AIFloat3::HashCode() const {
	static const int prime = 31;

	int result = 0;

	result = prime * result + ToIntBits(x);
	result = prime * result + ToIntBits(y);
	result = prime * result + ToIntBits(z);

	return result;
}*/

/*bool springai::AIFloat3::Equals(const void* obj) const {
	if (this == obj) {
		return true;
	} else if (obj == NULL) {
		return false;
	//} else if (!super.equals(obj)) {
	//	return false;
	//} else if (sizeof(*this) != sizeof(*obj)) {
	//	return false;
	}

	const AIFloat3& other = *((AIFloat3*)obj);
	if (ToIntBits(x) != ToIntBits(other.x)) {
		return false;
	} else if (ToIntBits(y) != ToIntBits(other.y)) {
		return false;
	} else if (ToIntBits(z) != ToIntBits(other.z)) {
		return false;
	} else {
		return true;
	}
}*/

const springai::AIFloat3 springai::AIFloat3::NULL_VALUE;

