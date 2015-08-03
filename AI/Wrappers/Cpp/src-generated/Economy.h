/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ECONOMY_H
#define _CPPWRAPPER_ECONOMY_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Economy {

public:
	virtual ~Economy(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual float GetCurrent(Resource* resource) = 0;

public:
	virtual float GetIncome(Resource* resource) = 0;

public:
	virtual float GetUsage(Resource* resource) = 0;

public:
	virtual float GetStorage(Resource* resource) = 0;

public:
	virtual float GetPull(Resource* resource) = 0;

public:
	virtual float GetShare(Resource* resource) = 0;

public:
	virtual float GetSent(Resource* resource) = 0;

public:
	virtual float GetReceived(Resource* resource) = 0;

public:
	virtual float GetExcess(Resource* resource) = 0;

	/**
	 * Give \<amount\> units of resource \<resourceId\> to team \<receivingTeam\>.
	 * - the amount is capped to the AI team's resource levels
	 * - does not check for alliance with \<receivingTeam\>
	 * - LuaRules might not allow resource transfers, AI's must verify the deduction
	 */
public:
	virtual bool SendResource(Resource* resource, float amount, int receivingTeamId) = 0;

	/**
	 * Give units specified by \<unitIds\> to team \<receivingTeam\>.
	 * \<ret_sentUnits\> represents how many actually were transferred.
	 * Make sure this always matches the size of \<unitIds\> you passed in.
	 * If it does not, then some unitId's were filtered out.
	 * - does not check for alliance with \<receivingTeam\>
	 * - AI's should check each unit if it is still under control of their
	 *   team after the transaction via UnitTaken() and UnitGiven(), since
	 *   LuaRules might block part of it
	 */
public:
	virtual int SendUnits(std::vector<springai::Unit*> unitIds_list, int receivingTeamId) = 0;

}; // class Economy

}  // namespace springai

#endif // _CPPWRAPPER_ECONOMY_H

