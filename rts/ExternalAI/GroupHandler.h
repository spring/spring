#ifndef GROUPHANDLER_H
#define GROUPHANDLER_H
// GroupHandler.h: interface for the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include <vector>
#include <set>
#include "aikey.h"
#include "Sim/Misc/GlobalConstants.h"

class CGroup;
class CUnitSet;

class CGroupHandler
{
public:
	CR_DECLARE(CGroupHandler);
	CGroupHandler(int team);
	virtual ~CGroupHandler();
	void PostLoad();

	void Update();
	void DrawCommands();
	void GroupCommand(int num);
	void GroupCommand(int num, const std::string& cmd);
	CGroup* CreateNewGroup(AIKey aiKey);
	void RemoveGroup(CGroup* group);
	void Load(std::istream *s);
	void Save(std::ostream *s);

	std::vector<CGroup*> groups;
	std::map<AIKey, std::string> availableAI;

	std::map<AIKey, std::string> GetSuitedAis(const CUnitSet& units);
	std::map<AIKey, std::string> lastSuitedAis;

	int team;
protected:
	void FindDlls(void);
	void TestDll(std::string name);

	std::vector<int> freeGroups;
	int firstUnusedGroup;
private:
	AIKey defaultKey;
};

//extern CGroupHandler* grouphandler;
extern CGroupHandler* grouphandlers[MAX_TEAMS];

#endif /* GROUPHANDLER_H */
