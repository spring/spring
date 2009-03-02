#ifndef AIRBASEHANDLER_H
#define AIRBASEHANDLER_H

#include <list>
#include <boost/noncopyable.hpp>

#include "Object.h"
#include "float3.h"
#include "GlobalConstants.h"

class CUnit;

class CAirBaseHandler : public boost::noncopyable
{
	CR_DECLARE(CAirBaseHandler);
	CR_DECLARE_SUB(LandingPad);
	CR_DECLARE_SUB(AirBase);

private:
	struct AirBase;

public:
	
	class LandingPad : public CObject,  public boost::noncopyable {
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

	struct AirBase  : public boost::noncopyable{
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
	std::vector< std::list<AirBase*> > freeBases;
	std::vector< std::list<AirBase*> > bases;

	typedef std::list<AirBase*> airBaseLst;
	typedef std::list<AirBase*>::iterator airBaseLstIt;
	typedef std::list<LandingPad*> padLst;
	typedef std::list<LandingPad*>::iterator padLstIt;
};

extern CAirBaseHandler* airBaseHandler;
#endif /* AIRBASEHANDLER_H */
