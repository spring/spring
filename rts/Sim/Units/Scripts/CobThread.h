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


class CCobThread : public CObject, public CUnitScript::IAnimListener
{
protected:
	std::string GetOpcodeName(int opcode);
	void LuaCall();
	// implementation of IAnimListener
	void AnimFinished(CUnitScript::AnimType type, int piece, int axis);

protected:
	CCobFile& script;
	CCobInstance* owner;

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

	inline int POP();
public:
	enum State {Init, Sleep, Run, Dead, WaitTurn, WaitMove};
	State state;
	int signalMask;

public:
	CCobThread(CCobFile& script, CCobInstance* owner);
	~CCobThread();
	bool Tick(int deltaTime);
	void Start(int functionId, const vector<int>& args, bool schedule);
	void SetCallback(CBCobThreadFinish cb, void* p1, void* p2);
	void DependentDied(CObject* o);
	int CheckStack(int size);
	int GetStackVal(int pos);
	const std::string& GetName();
	int GetWakeTime() const;
	void ShowError(const std::string& msg);
};

#endif // COB_THREAD_H
