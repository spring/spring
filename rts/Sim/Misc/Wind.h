/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WIND_H
#define WIND_H

#include <map>
#include "System/Misc/NonCopyable.h"

#include "System/float3.h"

class CUnit;

class CWind : public spring::noncopyable
{
	CR_DECLARE_STRUCT(CWind)

public:
	CWind();
	~CWind();

	void ResetState();
	void LoadWind(float min, float max);
	void Update();

	bool AddUnit(CUnit* u);
	bool DelUnit(CUnit* u);

	float GetMaxWind() const { return maxWind; }
	float GetMinWind() const { return minWind; }
	float GetCurrentStrength() const { return curStrength; }

	const float3& GetCurrentWind() const { return curWind; }
	const float3& GetCurrentDirection() const { return curDir; }

private:
	float maxWind;
	float minWind;
	float curStrength;

	float3 curDir;
	float3 curWind;
	float3 newWind;
	float3 oldWind;

	int status;

	std::map<int, CUnit*> windGens;
};

extern CWind wind;

#endif /* WIND_H */
