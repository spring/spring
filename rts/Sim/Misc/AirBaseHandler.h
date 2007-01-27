#ifndef AIRBASEHANDLER_H
#define AIRBASEHANDLER_H
#include "Object.h"
#include <list>

class CUnit;

class CAirBaseHandler :
	public CObject
{
public:
	CR_DECLARE(CAirBaseHandler)
	CR_DECLARE_SUB(LandingPad)
	CR_DECLARE_SUB(AirBase)

	CAirBaseHandler(void);
	~CAirBaseHandler(void);

	void RegisterAirBase(CUnit* base);
	void DeregisterAirBase(CUnit* base);

	struct AirBase;

	class LandingPad : public CObject{
	public:
		CR_DECLARE(LandingPad)
		CUnit* unit;
		AirBase* base;
		int piece;
	};

	struct AirBase {
		CR_DECLARE_STRUCT(AirBase)

		CUnit* unit;
		std::list<LandingPad*> freePads;
		std::list<LandingPad*> pads;
	};

	std::list<AirBase*> freeBases[MAX_TEAMS];
	std::list<AirBase*> bases[MAX_TEAMS];

	LandingPad* FindAirBase(CUnit* unit, float minPower);
	void LeaveLandingPad(LandingPad* pad);

	float3 FindClosestAirBasePos(CUnit* unit, float minPower);
};

extern CAirBaseHandler* airBaseHandler;
#endif /* AIRBASEHANDLER_H */
