// GroupAI.h: interface for the CGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUPAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GROUPAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "float3.h"
#include "ExternalAI/IGroupAI.h"
#include <set>
#include <map>

class IGroupAICallback;
class IAICallback;

const char AI_NAME[]="Simple formation";

using namespace std;

class CGroupAI : public IGroupAI  
{
public:
	CGroupAI();
	virtual ~CGroupAI();

	virtual void InitAi(IGroupAICallback* callback);

	virtual bool AddUnit(int unit);
	virtual void RemoveUnit(int unit);

	virtual void GiveCommand(Command* c);
	virtual const vector<CommandDescription>& GetPossibleCommands();
	virtual int GetDefaultCmd(int unitid);

	virtual void CommandFinished(int squad,int type){};

	virtual void Update();
	virtual void DrawCommands(){};

	set<int> myUnits;

	vector<CommandDescription> commands;
	IGroupAICallback* callback;
	IAICallback* aicb;

	bool unitsChanged;

	void MakeFormationMove(Command* c);
	void MoveToPos(int unit, float3& basePos, int posNum,unsigned char options);
	void GiveMoveOrder(int unit, const float3& pos,unsigned char options);

	float3 frontDir;
	float3 sideDir;
	float columnDist;
	int numColumns;
	float lineDist;
	float GetRotationFromVector(float3 vector);
	void CreateUnitOrder(std::multimap<float,int>& out);
};

#endif // !defined(AFX_GROUPAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
