/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBECONOMY_H
#define _CPPWRAPPER_STUBECONOMY_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Economy.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubEconomy : public Economy {

protected:
	virtual ~StubEconomy();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	// @Override
	virtual float GetCurrent(Resource* resource);
	// @Override
	virtual float GetIncome(Resource* resource);
	// @Override
	virtual float GetUsage(Resource* resource);
	// @Override
	virtual float GetStorage(Resource* resource);
	// @Override
	virtual float GetPull(Resource* resource);
	// @Override
	virtual float GetShare(Resource* resource);
	// @Override
	virtual float GetSent(Resource* resource);
	// @Override
	virtual float GetReceived(Resource* resource);
	// @Override
	virtual float GetExcess(Resource* resource);
	/**
	 * Give \<amount\> units of resource \<resourceId\> to team \<receivingTeam\>.
	 * - the amount is capped to the AI team's resource levels
	 * - does not check for alliance with \<receivingTeam\>
	 * - LuaRules might not allow resource transfers, AI's must verify the deduction
	 */
	// @Override
	virtual bool SendResource(Resource* resource, float amount, int receivingTeamId);
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
	// @Override
	virtual int SendUnits(std::vector<springai::Unit*> unitIds_list, int receivingTeamId);
}; // class StubEconomy

}  // namespace springai

#endif // _CPPWRAPPER_STUBECONOMY_H

