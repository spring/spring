/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AIR_BASE_HANDLER_H
#define AIR_BASE_HANDLER_H

#include <list>
#include <set>
#include <boost/noncopyable.hpp>

#include "System/Object.h"
#include "System/float3.h"

class CUnit;

class CAirBaseHandler : public boost::noncopyable
{
	CR_DECLARE(CAirBaseHandler);
	CR_DECLARE_SUB(LandingPad);
	CR_DECLARE_SUB(AirBase);

private:
	struct AirBase;

public:

	class LandingPad: public CObject, public boost::noncopyable {
		CR_DECLARE(LandingPad);

	public:
		LandingPad(CUnit* u, int p, AirBase* b):
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

	struct AirBase: public boost::noncopyable {
		CR_DECLARE_STRUCT(AirBase);
		AirBase(CUnit* u) : unit(u) {}

		CUnit* unit;
		std::list<LandingPad*> freePads;
		std::list<LandingPad*> pads;
	};

public:
	CAirBaseHandler();
	~CAirBaseHandler();

	void RegisterAirBase(CUnit* base);
	void DeregisterAirBase(CUnit* base);

	/**
	 * @brief Try to find an airbase and reserve it if one can be found
	 * Caller must call LeaveLandingPad() if it gets one
	 * and is finished with it or dies.
	 * It is the callers responsibility to detect if the base dies
	 * while its reserved.
	 */
	LandingPad* FindAirBase(CUnit* unit, float minPower);
	void LeaveLandingPad(LandingPad* pad);

	/** @brief Try to find the closest airbase even if it's reserved */
	float3 FindClosestAirBasePos(CUnit* unit, float minPower);

private:
	typedef std::list<AirBase*> AirBaseLst;
	typedef std::list<AirBase*>::iterator AirBaseLstIt;
	typedef std::list<LandingPad*> PadLst;
	typedef std::list<LandingPad*>::iterator PadLstIt;

	std::vector<AirBaseLst> freeBases;
	std::vector<AirBaseLst> bases;

	// IDs of units registered as airbases
	std::set<int> airBaseIDs;
};

extern CAirBaseHandler* airBaseHandler;

#endif /* AIR_BASE_HANDLER_H */
