/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef	_GROUP_HANDLER_H
#define	_GROUP_HANDLER_H

#include "System/creg/creg_cond.h"

#include <map>
#include <string>
#include <vector>
#include <set>

class CGroup;

/**
 * Handles all groups of a single team.
 * The default groups (aka hot-key groups) are the ones with
 * single digit IDs (1 - 9).
 * Groups with higher IDs are considered special.
 * Manual group creation/selection only works for the default groups.
 */
class CGroupHandler {
private:
	CR_DECLARE_STRUCT(CGroupHandler);

	CGroupHandler(int teamId);
	~CGroupHandler();

public:
	/// lowest ID of the first group not reachable through a hot-key
	static const size_t FIRST_SPECIAL_GROUP = 10;

	/**
	 * Initializes a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
//	static void Initialize(int teamId);
//	static CGroupHandler* GetInstance(int teamId);
//	static void Destroy(int teamId);

	void Update();

	void GroupCommand(int num);
	void GroupCommand(int num, const std::string& cmd);

	CGroup* CreateNewGroup();
	void RemoveGroup(CGroup* group);

	void PushGroupChange(int id);

public:
	std::vector<CGroup*> groups;
	int team;

protected:
	std::vector<int> freeGroups;
	/**
	 * The lowest ID not in use.
	 * This is always greater or equal FIRST_SPECIAL_GROUP.
	 */
	int firstUnusedGroup;
	std::set<int> changedGroups;
};

extern std::vector<CGroupHandler*> grouphandlers;

#endif	// _GROUP_HANDLER_H
