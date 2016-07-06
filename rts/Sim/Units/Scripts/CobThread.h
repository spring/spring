/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_THREAD_H
#define COB_THREAD_H

#include "CobInstance.h"
#include "Lua/LuaRules.h"

#include <string>
#include <vector>

class CCobFile;
class CCobInstance;

using std::vector;


class CCobThread
{
	CR_DECLARE_STRUCT(CCobThread)
	CR_DECLARE_SUB(CallInfo)
public:
	//creg only
	CCobThread();
	CCobThread(CCobInstance* owner);
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
	void SetCallback(CCobInstance::ThreadCallbackType cb, int cbp);
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

	int GetRetCode() const { return retCode; }
	bool IsWaiting() const { return (waitAxis != -1); }

	CCobInstance* owner;
protected:
	std::string GetOpcodeName(int opcode);
	void LuaCall();

	inline int POP();

	int wakeTime;
	int PC;
	vector<int> stack;
	//vector<int> execTrace;

	int paramCount;
	int retCode;

	int luaArgs[MAX_LUA_COB_ARGS];

	struct CallInfo {
		CR_DECLARE_STRUCT(CallInfo)
		int functionId;
		int returnAddr;
		int stackTop;
	};
	vector<CallInfo> callStack;

	CCobInstance::ThreadCallbackType cbType;
	int cbParam;


public:
	int waitAxis;
	int waitPiece;
	enum State {Init, Sleep, Run, Dead, WaitTurn, WaitMove};
	State state;
	int signalMask;
};

#endif // COB_THREAD_H
