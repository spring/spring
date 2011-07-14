/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __COMMAND_AI_H__
#define __COMMAND_AI_H__

#include <vector>
#include <set>

#include "System/Object.h"
#include "CommandQueue.h"
#include "System/float3.h"

class CUnit;
class CFeature;
class CWeapon;
class CLoadSaveInterface;
struct Command;

class CCommandAI : public CObject
{
	CR_DECLARE(CCommandAI);

public:
	CCommandAI(CUnit* owner);
	CCommandAI();
	virtual ~CCommandAI();
	void PostLoad();

	void DependentDied(CObject* o);
	/// feeds into GiveCommandReal()
	void GiveCommand(const Command& c, bool fromSynced = true);
	virtual int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	virtual void SlowUpdate();
	virtual void GiveCommandReal(const Command& c, bool fromSynced = true);
	virtual std::vector<CommandDescription>& GetPossibleCommands();
	virtual void FinishCommand();
	virtual void WeaponFired(CWeapon* weapon);
	virtual void BuggerOff(const float3& pos, float radius);
	virtual void LoadSave(CLoadSaveInterface* file, bool loading);
	/**
	 * @brief Determins if c will cancel a queued command
	 * @return true if c will cancel a queued command
	 */
	virtual bool WillCancelQueued(const Command& c);
	virtual bool CanSetMaxSpeed() const { return false; }
	virtual void StopMove() { return; }
	virtual bool HasMoreMoveCommands();
	/**
	 * Removes attack commands targeted at our new ally.
	 */
	virtual void StopAttackingAllyTeam(int ally);

	int CancelCommands(const Command& c, CCommandQueue& queue, bool& first);
	/**
	 * @brief Finds the queued command that would be canceled by the Command c
	 * @return An iterator pointing at the command, or commandQue.end(),
	 *   if no such queued command exsists
	 */
	CCommandQueue::iterator GetCancelQueued(const Command& c,
	                                        CCommandQueue& queue);
	/**
	 * @brief Returns commands that overlap c, but will not be canceled by c
	 * @return a vector containing commands that overlap c
	 */
	std::vector<Command> GetOverlapQueued(const Command& c);
	std::vector<Command> GetOverlapQueued(const Command& c,
	                                      CCommandQueue& queue);
	/**
	 * @brief Causes this CommandAI to execute the attack order c
	 */
	virtual void ExecuteAttack(Command& c);

	/**
	 * @brief executes the stop command c
	 */
	virtual void ExecuteStop(Command& c);

	void SetCommandDescParam0(const Command& c);
	bool ExecuteStateCommand(const Command& c);

	void ExecuteInsert(const Command& c, bool fromSynced = true);
	void ExecuteRemove(const Command& c);

	void AddStockpileWeapon(CWeapon* weapon);
	void StockpileChanged(CWeapon* weapon);
	void UpdateStockpileIcon();
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
	bool IsAttackCapable() const;
	virtual bool AllowedCommand(const Command& c, bool fromSynced);
	bool SkipParalyzeTarget(const CUnit* target);
	void GiveAllowedCommand(const Command& c, bool fromSynced = true);
	void GiveWaitCommand(const Command& c);
	/**
	 * @brief Returns the command that keeps the unit close to the path
	 * @return a Fight Command with 6 arguments, the first three being where to
	 *   return to (the current position of the unit), and the second being
	 *   the location of the origional command.
	 */
	void PushOrUpdateReturnFight(const float3& cmdPos1, const float3& cmdPos2);
	int UpdateTargetLostTimer(int unitid);
	void DrawDefaultCommand(const Command& c) const;

private:
	/**
	 * continously set to some non-zero value while target is in radar
	 * decremented every frame, command is canceled if it reaches 0
	 */
	int targetLostTimer;
};


#endif // __COMMAND_AI_H__
