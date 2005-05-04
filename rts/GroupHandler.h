// GroupHandler.h: interface for the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUPHANDLER_H__A41773E8_DB6A_4AF5_AE4C_E8380F7C7D04__INCLUDED_)
#define AFX_GROUPHANDLER_H__A41773E8_DB6A_4AF5_AE4C_E8380F7C7D04__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#include <string>
#include <vector>
class CGroup;

using namespace std;

class CGroupHandler  
{
public:
	CGroupHandler();
	virtual ~CGroupHandler();

	void Update();
	void GroupCommand(int num);
	CGroup* CreateNewGroup(string ainame);
	void RemoveGroup(CGroup* group);

	vector<CGroup*> armies;
	map<string,string> availableAI;

protected:
	void FindDlls(void);
	void TestDll(string name);

	vector<int> freeArmies;
	int firstUnusedGroup;
};

extern CGroupHandler* grouphandler;

#endif // !defined(AFX_GROUPHANDLER_H__A41773E8_DB6A_4AF5_AE4C_E8380F7C7D04__INCLUDED_)
