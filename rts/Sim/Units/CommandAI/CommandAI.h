#ifndef __COMMAND_AI_H__
#define __COMMAND_AI_H__

#include <vector>
#include <deque>
#include <set>

#include "Object.h"
#include "Command.h"
#include "CommandQueue.h"
#include "float3.h"

class CUnit;
class CFeature;
class CWeapon;
class CLoadSaveInterface;

class CCommandAI : public CObject
{
	CR_DECLARE(CCommandAI);

public:
	CCommandAI(CUnit* owner);
	CCommandAI();
	virtual ~CCommandAI(void);
	void PostLoad();

	void DependentDied(CObject* o);
	/// feeds into GiveCommandReal()
	void GiveCommand(const Command& c, bool fromSynced = true);
	virtual int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	virtual void SlowUpdate();
	virtual void GiveCommandReal(const Command& c, bool fromSynced = true);
	virtual std::vector<CommandDescription>& GetPossibleCommands();
	virtual void DrawCommands(void);
	virtual void FinishCommand(void);
	virtual void WeaponFired(CWeapon* weapon);
	virtual void BuggerOff(float3 pos, float radius);
	virtual void LoadSave(CLoadSaveInterface* file, bool loading);
	virtual bool WillCancelQueued(Command &c);
	virtual bool CanSetMaxSpeed() const { return false; }
	virtual void StopMove() { return; }
	virtual bool HasMoreMoveCommands();
	virtual void StopAttackingAllyTeam(int ally);

	int CancelCommands(const Command &c, CCommandQueue& queue, bool& first);
	CCommandQueue::iterator GetCancelQueued(const Command &c,
	                                        CCommandQueue& queue);
	std::vector<Command> GetOverlapQueued(const Command &c);
	std::vector<Command> GetOverlapQueued(const Command &c,
	                                      CCommandQueue& queue);
	virtual void ExecuteAttack(Command &c);
	virtual void ExecuteDGun(Command &c);
	virtual void ExecuteStop(Command &c);

	void SetCommandDescParam0(const Command& c);
	bool ExecuteStateCommand(const Command& c);

	void ExecuteInsert(const Command& c, bool fromSynced = true);
	void ExecuteRemove(const Command& c);

	void AddStockpileWeapon(CWeapon* weapon);
	void StockpileChanged(CWeapon* weapon);
	void UpdateStockpileIcon(void);
	bool CanChangeFireState();
	CWeapon* stockpileWeapon;

	std::vector<CommandDescription> possibleCommands;
	CCommandQueue commandQue;
	/**
	 * commands that will not go into the command queue
	 * (and therefore not reseting it if given without shift
	 */
	std::set<int> nonQueingCommands;
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
	bool isTrackable(const CUnit* unit) const;
	bool isAttackCapable() const;
	virtual bool AllowedCommand(const Command &c, bool fromSynced);
	bool SkipParalyzeTarget(const CUnit* target);
	void GiveAllowedCommand(const Command& c, bool fromSynced = true);
	void GiveWaitCommand(const Command& c);
	void PushOrUpdateReturnFight(const float3& cmdPos1, const float3& cmdPos2);
	int UpdateTargetLostTimer(int unitid);
	void DrawWaitIcon(const Command& cmd) const;
	void DrawDefaultCommand(const Command& c) const;

private:
	/**
	 * continously set to some non-zero value while target is in radar
	 * decremented every frame, command is canceled if it reaches 0
	 */
	int targetLostTimer;
};


#endif // __COMMAND_AI_H__
