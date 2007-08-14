#ifndef WIND_H
#define WIND_H

#include "float3.h"

class CWind
{
	NO_COPY(CWind);
	CR_DECLARE(CWind);

public:
	CWind();
	~CWind();

	void LoadWind();
	void Update();

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
};

extern CWind wind;

#endif /* WIND_H */
