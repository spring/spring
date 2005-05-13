// Group.h: interface for the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUP_H__5CCDFEDA_0AB1_425F_B8D1_5B7BC794EB93__INCLUDED_)
#define AFX_GROUP_H__5CCDFEDA_0AB1_425F_B8D1_5B7BC794EB93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "Object.h"
#include <string>
#include <set>
#include "command.h"
class IGroupAI;
class CUnit;
class CFeature;
class CGroupAiCallback;

using namespace std;

class CGroup : public CObject  
{
public:
	void CommandFinished(int unit,int type);
	CGroup(string dllName,int id);
	virtual ~CGroup();

	void Update();
	void SetNewAI(string dllName);

	void RemoveUnit(CUnit* unit);
	bool AddUnit(CUnit* unit);		//dont call this directly call unit.SetGroup and let that call this
	const vector<CommandDescription>& GetPossibleCommands();
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	void GiveCommand(Command c);
	void ClearUnits(void);

	int id;

	set<CUnit*> units;

	vector<CommandDescription> myCommands;
#ifndef NO_WINSTUFF
	HINSTANCE m_hDLL;
	typedef int (WINAPI* GETGROUPAIVERSION)();
	typedef IGroupAI* (WINAPI* GETNEWAI)();
	typedef void (WINAPI* RELEASEAI)(IGroupAI* i);
	
	GETGROUPAIVERSION GetGroupAiVersion;
	GETNEWAI GetNewAI;
	RELEASEAI ReleaseAI;
	IGroupAI* ai;
#else
#warning this file is full of garbage just to make it compile, please port
	//FIXME AWFULL COMPILATION FIX -> port me
        void * m_hDLL;
        typedef int (* GETGROUPAIVERSION)();
        typedef IGroupAI* (* GETNEWAI)();
        typedef void (* RELEASEAI)(IGroupAI* i);
        GETGROUPAIVERSION GetGroupAiVersion;
        GETNEWAI GetNewAI;
        RELEASEAI ReleaseAI;
        IGroupAI* ai;								
#endif
	int lastCommandPage;
	int currentAiNum;

	CGroupAiCallback* callback;

};

#endif // !defined(AFX_GROUP_H__5CCDFEDA_0AB1_425F_B8D1_5B7BC794EB93__INCLUDED_)

