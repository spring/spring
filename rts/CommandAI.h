#pragma once
#include "object.h"
#include "command.h"
#include <vector>
#include <deque>
#include <set>

class CUnit;
class CFeature;
class CWeapon;
class CLoadSaveInterface;

class CCommandAI :
	public CObject
{
public:
	CCommandAI(CUnit* owner);
	virtual ~CCommandAI(void);

	void DependentDied(CObject* o);
	virtual int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	virtual void SlowUpdate();
	virtual void GiveCommand(Command& c);
	virtual vector<CommandDescription>& GetPossibleCommands();
	virtual void DrawCommands(void);
	virtual void FinishCommand(void);
	virtual void WeaponFired(CWeapon* weapon);
	virtual void BuggerOff(float3 pos, float radius);
	virtual void LoadSave(CLoadSaveInterface* file, bool loading);

	void AddStockpileWeapon(CWeapon* weapon);
	void StockpileChanged(CWeapon* weapon);
	void UpdateStockpileIcon(void);
	CWeapon* stockpileWeapon;

	vector<CommandDescription> possibleCommands;
	deque<Command> commandQue;
	set<int> nonQueingCommands;			//commands that wont go into the command que (and therefore not reseting it if given without shift
	int lastUserCommand;
	int selfDCountdown;
	int lastFinishCommand;

	CUnit* owner;

	CUnit* orderTarget;
	bool targetDied;
	bool inCommand;
	bool selected;
	bool repeatOrders;
	int lastSelectedCommandPage;
	bool unimportantMove;
};
