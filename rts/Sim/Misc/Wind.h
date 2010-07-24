/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WIND_H
#define WIND_H

#include <map>
#include <boost/noncopyable.hpp>

#include "float3.h"

class CUnit;
class CWind : public boost::noncopyable
{
	CR_DECLARE(CWind);

public:
	CWind();
	~CWind();

	void LoadWind(float min, float max);
	void Update();

	bool AddUnit(CUnit*);
	bool DelUnit(CUnit*);

	float GetMaxWind() const { return maxWind; }
	float GetMinWind() const { return minWind; }

	const float3& GetCurrentWind() const { return curWind; }
	float GetCurrentStrength() const { return curStrength; }
	const float3& GetCurrentDirection() const { return curDir; }

private:
	float maxWind;
	float minWind;

	float3 curWind;
	float curStrength;
	float3 curDir;

	float3 newWind;
	float3 oldWind;
	int status;

	std::map<int, CUnit*> windGens;
};

extern CWind wind;

#endif /* WIND_H */
