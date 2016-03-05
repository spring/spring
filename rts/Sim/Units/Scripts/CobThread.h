/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_THREAD_H
#define COB_THREAD_H

#include "CobInstance.h"
#include "System/Object.h"
#include "Lua/LuaRules.h"

#include <string>
#include <vector>

class CCobFile;
class CCobInstance;

using std::vector;


class CCobThread : public CObject
{
public:
	CCobThread(CCobFile& script, CCobInstance* owner);
	/// Inform the vultures that we finally croaked
	~CCobThread();

	/**
	 * Returns false if this thread is dead and needs to be killed.
	 */
	bool Tick();
	/**
	 * This function sets the thread in motion. Should only be called once.
	 * If schedule is false the thread is not added to the scheduler, and thus
	 * it is expected that the starter is responsible for ticking it.
	 */
	void Start(int functionId, const vector<int>& args, bool schedule);
	/**
	 * Sets a callback that will be called when the thread dies.
	 * There can be only one.
	 */
	void SetCallback(CBCobThreadFinish cb, void* p1, void* p2);
	/**
	 * @brief Checks whether the stack has at least size items.
	 * @returns min(size, stack.size())
	 */
	int CheckStack(unsigned int size, bool warn);
	/**
	 * @brief Returns the value at pos in the stack.
	 * Neater than exposing the actual stack
	 */
	int GetStackVal(int pos);
	const std::string& GetName();
	int GetWakeTime() const;
	/**
	 * Shows an errormessage which includes the current state of the script
	 * interpreter.
	 */
	void ShowError(const std::string& msg);
	void AnimFinished(CUnitScript::AnimType type, int piece, int axis);

	CCobInstance* owner;
protected:
	std::string GetOpcodeName(int opcode);
	void LuaCall();

	inline int POP();

	CCobFile& script;

	int wakeTime;
	int PC;
	vector<int> stack;
	vector<int> execTrace;

	int paramCount;
	int retCode;

	int luaArgs[MAX_LUA_COB_ARGS];

	struct callInfo {
		int functionId;
		int returnAddr;
		size_t stackTop;
	};
	vector<struct callInfo> callStack;

	CBCobThreadFinish callback;
	void* cbParam1;
	void* cbParam2;


public:
	int waitAxis;
	int waitPiece;
	enum State {Init, Sleep, Run, Dead, WaitTurn, WaitMove};
	State state;
	int signalMask;
};

#endif // COB_THREAD_H
