/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef	_GROUP_HANDLER_H
#define	_GROUP_HANDLER_H

#include "Group.h"
#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"

#include <string>
#include <vector>

class CGroup;

/**
 * Handles all groups of a single team.
 * The default groups (aka hot-key groups) are the ones with
 * single digit IDs (1 - 9).
 * Groups with higher IDs are considered special.
 * Manual group creation/selection only works for the default groups.
 */
class CGroupHandler {
	CR_DECLARE_STRUCT(CGroupHandler)
public:
	CGroupHandler(int teamId);

	CGroupHandler(const CGroupHandler& ) = delete;
	CGroupHandler(      CGroupHandler&&) = default;

	~CGroupHandler() = default;

	CGroupHandler& operator = (const CGroupHandler& ) = delete;
	CGroupHandler& operator = (      CGroupHandler&&) = default;

	/// lowest ID of the first group not reachable through a hot-key
	static constexpr size_t FIRST_SPECIAL_GROUP = 10;

	void Update();

	bool GroupCommand(int num);
	bool GroupCommand(int num, const std::string& cmd);

	// NOTE: only invoked by AI's, but can invalidate pointers
	CGroup* CreateNewGroup();
	CGroup* GetUnitGroup(int unitID) {
		const auto iter = unitGroups.find(unitID);

		if (iter == unitGroups.end())
			return nullptr;

		return &groups[iter->second];
	}

	const CGroup* GetGroup(int groupID) const { return &groups[groupID]; }
	      CGroup* GetGroup(int groupID)       { return &groups[groupID]; }

	const std::vector<CGroup>& GetGroups() const { return groups; }

	bool HasGroup(int groupID) const { return (groupID >= 0 && groupID < groups.size()); }
	bool SetUnitGroup(int unitID, const CGroup* g) {
		unitGroups.erase(unitID);

		if (g == nullptr)
			return false;

		unitGroups.insert(unitID, g->id);
		return true;
	}

	void RemoveGroup(CGroup* group);

	void PushGroupChange(int id);

	int GetTeam() const { return team; }
	int GetGroupSize(int groupID) const { return (groups[groupID].units.size()); }

private:
	std::vector<CGroup> groups;

	std::vector<int> freeGroups;
	std::vector<int> changedGroups;

	spring::unsynced_map<int, int> unitGroups;

	int team = 0;
	// lowest ID not in use, always >= FIRST_SPECIAL_GROUP
	int firstUnusedGroup = FIRST_SPECIAL_GROUP;

};

extern std::vector<CGroupHandler> uiGroupHandlers;

#endif	// _GROUP_HANDLER_H
