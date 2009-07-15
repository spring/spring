#ifndef __COB_THREAD_H__
#define __COB_THREAD_H__

#include <vector>
#include "Object.h"
#include "CobInstance.h"
#include "Lua/LuaRules.h"
#include "LogOutput.h"

class CCobFile;
class CCobInstance;

class CCobThread : public CObject, public CUnitScript::IAnimListener
{
protected:
	string GetOpcodeName(int opcode);
	void ForceCommitAnim(int type, int piece, int axis);
	void ForceCommitAllAnims();
	void LuaCall();
	// implementation of IAnimListener
	void AnimFinished(CUnitScript::AnimType type, int piece, int axis);

protected:
	CCobFile &script;
	CCobInstance *owner;

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
	void *cbParam1;
	void *cbParam2;

	struct DelayedAnim {
		int type;
		int piece;
		int axis;
		int dest;
	};
	vector<DelayedAnim> delayedAnims;

	inline int POP(void);
public:
	enum State {Init, Sleep, Run, Dead, WaitTurn, WaitMove};
	State state;
	int signalMask;

public:
	CCobThread(CCobFile &script, CCobInstance *owner);
	~CCobThread(void);
	int Tick(int deltaTime);
	void Start(int functionId, const vector<int> &args, bool schedule);
	void SetCallback(CBCobThreadFinish cb, void *p1, void *p2);
	void DependentDied(CObject* o);
	int CheckStack(int size);
	int GetStackVal(int pos);
	const string &GetName();
	int GetWakeTime() const;
	void CommitAnims(int deltaTime);
	void ShowError(const string& msg);
};

#endif // __COB_THREAD_H__
