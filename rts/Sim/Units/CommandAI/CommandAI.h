#ifndef __COMMAND_AI_H__
#define __COMMAND_AI_H__

#include "Object.h"
#include "Game/command.h"
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
	virtual void GiveCommand(const Command& c);
	virtual vector<CommandDescription>& GetPossibleCommands();
	virtual void DrawCommands(void);
	virtual void FinishCommand(void);
	virtual void WeaponFired(CWeapon* weapon);
	virtual void BuggerOff(float3 pos, float radius);
	virtual void LoadSave(CLoadSaveInterface* file, bool loading);
	virtual bool WillCancelQueued(Command &c);
	virtual bool CanSetMaxSpeed() const { return false; }
	virtual void StopMove() { return; }
	std::deque<Command>::iterator GetCancelQueued(const Command &c);
	std::vector<Command> GetOverlapQueued(const Command &c);

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

protected:
	bool AllowedCommand(const Command &c);
	void GiveAllowedCommand(const Command& c);
	void PushOrUpdateReturnFight(const float3& cmdPos1, const float3& cmdPos2);
};

#endif // __COMMAND_AI_H__
