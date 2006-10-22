#ifndef GROUPHANDLER_H
#define GROUPHANDLER_H
// GroupHandler.h: interface for the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <map>
#include <string>
#include <vector>
#include <set>
#include "ExternalAI/aikey.h"

class CGroup;

using namespace std;

class CGroupHandler  
{
public:
	CGroupHandler(int team);
	virtual ~CGroupHandler();

	void Update();
	void DrawCommands();
	void GroupCommand(int num);
	CGroup* CreateNewGroup(AIKey aiKey);
	void RemoveGroup(CGroup* group);

	vector<CGroup*> groups;
	map<AIKey,string> availableAI;

	map<AIKey,string> GetSuitedAis(set<CUnit*> units);
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

extern CGroupHandler* grouphandler;

#endif /* GROUPHANDLER_H */
