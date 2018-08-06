/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_THREAD_H
#define COB_THREAD_H

#include <string>
#include <array>

#include "CobInstance.h"
#include "Lua/LuaRules.h"

class CCobFile;
class CCobInstance;


class CCobThread
{
	CR_DECLARE_STRUCT(CCobThread)
	CR_DECLARE_SUB(CallInfo)

public:
	// default and copy-ctor are creg only
	CCobThread() {}

	CCobThread(CCobInstance* _cobInst);
	CCobThread(CCobThread&& t) { *this = std::move(t); }
	CCobThread(const CCobThread& t) { *this = t; }

	~CCobThread() { Stop(); }

	CCobThread& operator = (CCobThread&& t);
	CCobThread& operator = (const CCobThread& t);

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
	void Start(int functionId, int sigMask, const std::array<int, 1 + MAX_COB_ARGS>& args, bool schedule);
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
		cobInst = nullptr;
		cobFile = nullptr;
	}


	/**
	 * @brief Checks whether the stack has at least size items.
	 * @returns min(size, stack.size())
	 */
	int CheckStack(unsigned int size, bool warn);
	void InitStack(unsigned int n, CCobThread* t);

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
	bool IsGarbage() const { return (cobInst == nullptr); }
	bool IsWaiting() const { return (waitAxis != -1); }

	// script instance that owns this thread
	CCobInstance* cobInst = nullptr;
	CCobFile* cobFile = nullptr;

protected:
	struct CallInfo {
		CR_DECLARE_STRUCT(CallInfo)
		int functionId = -1;
		int returnAddr = -1;
		int stackTop = -1;
	};

	void LuaCall();

	bool PushCallStack(CallInfo v) { return (callStackSize < callStack.size() && PushCallStackRaw(v)); }
	bool PushDataStack(     int v) { return (dataStackSize < dataStack.size() && PushDataStackRaw(v)); }

	bool PushCallStackRaw(CallInfo v) { assert(callStackSize < callStack.size()); callStack[callStackSize++] = v; return true; }
	bool PushDataStackRaw(     int v) { assert(dataStackSize < dataStack.size()); dataStack[dataStackSize++] = v; return true; }

	CallInfo& PushCallStackRef() {
		if (callStackSize < callStack.size())
			return (PushCallStackRefRaw());
		return callStack[0];
	}
	CallInfo& PushCallStackRefRaw() {
		assert(callStackSize < callStack.size());
		return callStack[callStackSize++];
	}

	int LocalFunctionID() const { return callStack[callStackSize - (callStackSize > 0)].functionId; }
	int LocalReturnAddr() const { return callStack[callStackSize - (callStackSize > 0)].returnAddr; }
	int LocalStackFrame() const { return callStack[callStackSize - (callStackSize > 0)].stackTop  ; }

	int PopDataStack() {
		if (dataStackSize > 0)
			return dataStack[--dataStackSize];

		return 0;
	}

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

	int callStackSize = 0;
	int dataStackSize = 0;

	int errorCounter = 100;

	int luaArgs[MAX_LUA_COB_ARGS] = {0};


	std::array<CallInfo, 64> callStack;
	std::array<int, 1024> dataStack;
	// std::vector<int> execTrace;

	State state = Init;

	CCobInstance::ThreadCallbackType cbType = CCobInstance::CBNone;
};

#endif // COB_THREAD_H
