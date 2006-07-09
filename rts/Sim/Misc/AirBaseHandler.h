#ifndef AIRBASEHANDLER_H
#define AIRBASEHANDLER_H
#include "Object.h"
#include <list>

class CUnit;

class CAirBaseHandler :
	public CObject
{
public:
	CAirBaseHandler(void);
	~CAirBaseHandler(void);

	void RegisterAirBase(CUnit* base);
	void DeregisterAirBase(CUnit* base);

	struct AirBase;

	class LandingPad : public CObject{
	public:
		CUnit* unit;
		AirBase* base;
		int piece;
	};

	struct AirBase {
		CUnit* unit;
		std::list<LandingPad*> freePads;
		std::list<LandingPad*> pads;
	};

	std::list<AirBase*> freeBases[MAX_TEAMS];
	std::list<AirBase*> bases[MAX_TEAMS];

	LandingPad* FindAirBase(CUnit* unit, float maxDist, float minPower);
	void LeaveLandingPad(LandingPad* pad);

	float3 FindClosestAirBasePos(CUnit* unit, float maxDist, float minPower);
};

extern CAirBaseHandler* airBaseHandler;
#endif /* AIRBASEHANDLER_H */
