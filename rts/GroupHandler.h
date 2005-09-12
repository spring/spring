#ifndef GROUPHANDLER_H
#define GROUPHANDLER_H
// GroupHandler.h: interface for the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <map>
#include <string>
#include <vector>
class CGroup;

using namespace std;

class CGroupHandler  
{
public:
	CGroupHandler(int team);
	virtual ~CGroupHandler();

	void Update();
	void GroupCommand(int num);
	CGroup* CreateNewGroup(string ainame);
	void RemoveGroup(CGroup* group);

	vector<CGroup*> groups;
	map<string,string> availableAI;

	int team;
protected:
	void FindDlls(void);
	void TestDll(string name);

	vector<int> freeGroups;
	int firstUnusedGroup;
};

extern CGroupHandler* grouphandler;

#endif /* GROUPHANDLER_H */
