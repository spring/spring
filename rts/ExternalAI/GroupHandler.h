#ifndef GROUPHANDLER_H
#define GROUPHANDLER_H
// GroupHandler.h: interface for the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <map>
#include <string>
#include <vector>
#include <set>
#include "ExternalAI/aikey.h"

class CGroup;
class CUnitSet;

using namespace std;

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
	void GroupCommand(int num, const string& cmd);
	CGroup* CreateNewGroup(AIKey aiKey);
	void RemoveGroup(CGroup* group);
	void Load(std::istream *s);
	void Save(std::ostream *s);

	vector<CGroup*> groups;
	map<AIKey,string> availableAI;

	map<AIKey,string> GetSuitedAis(const CUnitSet& units);
	map<AIKey,string> lastSuitedAis;

	int team;
protected:
	void FindDlls(void);
	void TestDll(string name);

	vector<int> freeGroups;
	int firstUnusedGroup;
private:
	AIKey defaultKey;
};

//extern CGroupHandler* grouphandler;
extern CGroupHandler* grouphandlers[MAX_TEAMS];

#endif /* GROUPHANDLER_H */
