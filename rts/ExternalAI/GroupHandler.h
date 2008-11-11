/*
	@author ???
	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef	_GROUPHANDLER_H
#define	_GROUPHANDLER_H

#include <map>
#include <string>
#include <vector>
#include <set>
//#include "ExternalAI/aikey.h"
#include "Game/GlobalConstants.h"

class CGroup;
//class CUnitSet;

/**
 * Handles All Groups of a single team.
 * NOTE: Does not handle GroupAIs anymore, this is done by the groups themself.
 */
class CGroupHandler {
private:
	CR_DECLARE(CGroupHandler);
	CGroupHandler(int teamId);
	virtual ~CGroupHandler();

public:

	/**
	 * This function initialized a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
	static void Initialize(int teamId);
	static CGroupHandler* GetInstance(int teamId);
	static void Destroy(int teamId);

	void PostLoad();

	void Update();
	void DrawCommands();
	void GroupCommand(int num);
	void GroupCommand(int num, const std::string& cmd);
//	CGroup* CreateNewGroup(AIKey aiKey);
	void RemoveGroup(CGroup* group);
	void Load(std::istream *s);
	void Save(std::ostream *s);

//	std::map<AIKey, std::string> availableAI;

//	std::map<AIKey, std::string> GetSuitedAis(const CUnitSet& units);
//	std::map<AIKey, std::string> lastSuitedAis;

protected:
	int teamId;
	std::vector<CGroup*> groups;
//	void FindDlls(void);
//	void TestDll(std::string name);

	std::vector<int> freeGroups;
	int firstUnusedGroup;
private:
//	AIKey defaultKey;
};

//extern CGroupHandler* grouphandler;
//extern CGroupHandler* grouphandlers[MAX_TEAMS];

#endif	// _GROUPHANDLER_H
