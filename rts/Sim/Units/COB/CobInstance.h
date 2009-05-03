#ifndef __COB_INSTANCE_H__
#define __COB_INSTANCE_H__

#include "Sim/Misc/GlobalConstants.h"
#include "UnitScript.h"


class CCobThread;
class CCobFile;
class CCobInstance;

class CCobInstance : public CUnitScript
{
protected:
	CCobFile& script;

	virtual int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	virtual void ShowScriptError(const std::string& msg);
	virtual void ShowScriptWarning(const std::string& msg);

public:
	std::vector<int> staticVars;
	std::list<CCobThread *> threads;
	bool smoothAnim;
	const CCobFile* GetScriptAddr() const { return &script; }

public:
	const int* GetUnitVars() const { return unitVars; };

	static const int* GetTeamVars(int team) { return &teamVars[team][0]; }
	static const int* GetAllyVars(int ally) { return &allyVars[ally][0]; }
	static const int* GetGlobalVars()       { return globalVars; }

public:
	CCobInstance(CCobFile &script, CUnit *unit);
	~CCobInstance();

	virtual int GetFunctionId(const std::string& fname) const;

	// used by CCobThread
	void Signal(int signal);
	void PlayUnitSound(int snr, int attr);
};

#endif // __COB_INSTANCE_H__
