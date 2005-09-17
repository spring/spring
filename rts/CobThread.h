#ifndef __COB_THREAD_H__
#define __COB_THREAD_H__

#include "Object.h"
#include "CobInstance.h"
#include <vector>
#include "InfoConsole.h"

class CCobFile;
class CCobInstance;

using namespace std;

class CCobThread : public CObject
{
protected:
	CCobFile &script;
	CCobInstance *owner;

	int wakeTime;
	long PC;
	vector<long> stack;
	vector<long> execTrace;

	int paramCount;
	long retCode;

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
	long signalMask;
public:
	CCobThread(CCobFile &script, CCobInstance *owner);
	~CCobThread(void);
	int Tick(int deltaTime);
	void Start(int functionId, const vector<long> &args, bool schedule);
	void SetCallback(CBCobThreadFinish cb, void *p1, void *p2);
	string GetOpcodeName(int opcode);
	void DependentDied(CObject* o);
	int GetStackVal(int pos);
	const string &GetName();
	int GetWakeTime() const;
	void ForceCommitAnim(int type, int piece, int axis);
	void ForceCommitAllAnims();
	void CommitAnims(int deltaTime);
	void ShowError(const string& msg);
};

#endif // __COB_THREAD_H__
