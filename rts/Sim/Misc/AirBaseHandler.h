#ifndef AIRBASEHANDLER_H
#define AIRBASEHANDLER_H

#include "Object.h"
#include <list>

class CUnit;

class CAirBaseHandler
{
	NO_COPY(CAirBaseHandler);
	CR_DECLARE(CAirBaseHandler);
	CR_DECLARE_SUB(LandingPad);
	CR_DECLARE_SUB(AirBase);

private:
	struct AirBase;

public:

	class LandingPad : public CObject {
		NO_COPY(LandingPad);
		CR_DECLARE(LandingPad);

	public:
		LandingPad(CUnit* u, int p, AirBase* b) :
			unit(u), piece(p), base(b) {}

		CUnit* GetUnit() const { return unit; }
		int GetPiece() const { return piece; }
		AirBase* GetBase() const { return base; }

	private:
		CUnit* unit;
		int piece;
		AirBase* base;
	};

private:

	struct AirBase {
		NO_COPY(AirBase);
		CR_DECLARE_STRUCT(AirBase);
		AirBase(CUnit* u) : unit(u) {}
		CUnit* unit;
		std::list<LandingPad*> freePads;
		std::list<LandingPad*> pads;
	};

public:
	CAirBaseHandler(void);
	~CAirBaseHandler(void);

	void RegisterAirBase(CUnit* base);
	void DeregisterAirBase(CUnit* base);

	LandingPad* FindAirBase(CUnit* unit, float minPower);
	void LeaveLandingPad(LandingPad* pad);

	float3 FindClosestAirBasePos(CUnit* unit, float minPower);

private:
	std::list<AirBase*> freeBases[MAX_TEAMS];
	std::list<AirBase*> bases[MAX_TEAMS];
};

extern CAirBaseHandler* airBaseHandler;
#endif /* AIRBASEHANDLER_H */
