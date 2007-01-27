#ifndef WIND_H
#define WIND_H

#include "float3.h"

class CWind
{
public:
	CR_DECLARE(CWind);

	CWind();
	~CWind();
	void LoadWind();
	void Update();

	float maxWind;
	float minWind;


	float3 curWind;
	float curStrength;
	float3 curDir;

protected:
	float3 newWind;
	float3 oldWind;
	int status;
};

extern CWind wind;

#endif /* WIND_H */
