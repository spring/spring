#pragma once

#include "float3.h"

class CWind
{
public:
	CWind(void);
	~CWind(void);
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