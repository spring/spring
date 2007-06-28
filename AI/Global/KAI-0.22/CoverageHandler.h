#ifndef COVERAGE_HANDLER_H
#define COVERAGE_HANDLER_H

#include "GlobalAI.h"

class CCoverageHandler{
public:
	CR_DECLARE(CCoverageHandler);
	typedef enum CoverageType{
		Radar,
		Sonar,
		RJammer,
		SJammer,
		AntiNuke,
		Shield
	};
    CCoverageHandler(AIClasses* _ai,CoverageType type);
	~CCoverageHandler();
	
//	void Change(int unit);
	float3 NextSite(int builder, const UnitDef* unit, int MaxDist);
	int GetCoverage(float3 pos);

	void Change(int unit, bool Removed);

private:
	vector<unsigned char> CovMap;
    AIClasses* ai;
	CoverageType Type;
};
#endif
