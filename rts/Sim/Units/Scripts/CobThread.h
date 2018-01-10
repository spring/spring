/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_THREAD_H
#define COB_THREAD_H

#include "CobInstance.h"
#include "Lua/LuaRules.h"

#include <string>
#include <vector>

class CCobFile;
class CCobInstance;


class CCobThread
{
	CR_DECLARE_STRUCT(CCobThread)
	CR_DECLARE_SUB(CallInfo)

public:
	// creg only
	CCobThread() {}
	CCobThread(CCobInstance* owner);
	CCobThread(CCobThread&& t) { *this = std::move(t); }
	~CCobThread() { Stop(); }

	CCobThread& operator = (CCobThread&& t);

	enum State {Init, Sleep, Run, Dead, WaitTurn, WaitMove};

	/**
	 * Returns false if this thread is dead and needs to be killed.
	 */
	bool Tick();
	/**
	 * This function sets the thread in motion. Should only be called once.
	 * If schedule is false the thread is not added to the scheduler, and thus
	 * it is expected that the starter is responsible for ticking it.
	 */
	void Start(int functionId, int sigMask, const std::vector<int>& args, bool schedule);
	void Stop();

	void SetID(int threadID) { id = threadID; }
	void SetState(State s) { state = s; }

	/**
	 * Sets a callback that will be called when the thread dies.
	 * There can be only one.
	 */
	void SetCallback(CCobInstance::ThreadCallbackType cb, int cbp) {
		cbType = cb;
		cbParam = cbp;
	}
	void MakeGarbage() {
		owner = nullptr;
		cob = nullptr;
	}


	/**
	 * @brief Checks whether the stack has at least size items.
	 * @returns min(size, stack.size())
	 */
	int CheckStack(unsigned int size, bool warn);

	/**
	 * Shows an errormessage which includes the current state of the script
	 * interpreter.
	 */
	void ShowError(const char* msg);
	void AnimFinished(CUnitScript::AnimType type, int piece, int axis);

	const std::string& GetName();

	int GetID() const { return id; }
	int GetStackVal(int pos) const { return dataStack[pos]; }
	int GetWakeTime() const { return wakeTime; }
	int GetRetCode() const { return retCode; }
	int GetSignalMask() const { return signalMask; }
	State GetState() const { return state; }

	bool Reschedule(CUnitScript::AnimType type) const {
		return ((state == WaitMove && type == CCobInstance::AMove) || (state == WaitTurn && type == CCobInstance::ATurn));
	}

	bool IsDead() const { return (state == Dead); }
	bool IsGarbage() const { return (owner == nullptr); }
	bool IsWaiting() const { return (waitAxis != -1); }

	CCobInstance* owner = nullptr;
	CCobFile* cob = nullptr;

protected:
	void LuaCall();

	inline int POP();

protected:
	int id = -1;
	int pc = 0;

	int wakeTime = 0;
	int paramCount = 0;
	int retCode = -1;
	int cbParam = 0;
	int signalMask = 0;

	int waitAxis = -1;
	int waitPiece = -1;

	int errorCounter = 100;

	int luaArgs[MAX_LUA_COB_ARGS] = {0};

	struct CallInfo {
		CR_DECLARE_STRUCT(CallInfo)
		int functionId;
		int returnAddr;
		int stackTop;
	};

	std::vector<CallInfo> callStack;
	std::vector<int> dataStack;
	std::vector<int> callArgs;
	// std::vector<int> execTrace;

	State state = Init;

	CCobInstance::ThreadCallbackType cbType;
};

#endif // COB_THREAD_H
