/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CPPWRAPPER_AI_FLOAT_3_H
#define _CPPWRAPPER_AI_FLOAT_3_H

#include <string>

#include "System/float3.h"

namespace springai {

/**
 * Represents a position on the map.
 */
class AIFloat3 : public float3 {
public:

	AIFloat3();
	AIFloat3(float x, float y, float z);
	AIFloat3(float* xyz);
	AIFloat3(const AIFloat3& other);
	AIFloat3(const float3& f3);

	void LoadInto(float* xyz) const;

	virtual std::string ToString() const;
//	virtual int HashCode() const;
//	virtual bool Equals(const void* obj) const;

	static const AIFloat3 NULL_VALUE;
}; // class AIFloat3

}  // namespace springai

#endif // _CPPWRAPPER_AI_FLOAT_3_H
