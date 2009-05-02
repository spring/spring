#ifndef __COB_INSTANCE_H__
#define __COB_INSTANCE_H__

#include "Sim/Misc/GlobalConstants.h"
#include "UnitScript.h"


#define PACKXZ(x,z) (((int)(x) << 16)+((int)(z) & 0xffff))
#define UNPACKX(xz) ((signed short)((boost::uint32_t)(xz) >> 16))
#define UNPACKZ(xz) ((signed short)((boost::uint32_t)(xz) & 0xffff))


class CCobThread;
class CCobFile;
class CCobInstance;

class CCobInstance : public CUnitScript
{
public:
	static const int UNIT_VAR_COUNT   = 8;
	static const int TEAM_VAR_COUNT   = 64;
	static const int ALLY_VAR_COUNT   = 64;
	static const int GLOBAL_VAR_COUNT = 4096;

	static const int UNIT_VAR_START   = 1024;
	static const int TEAM_VAR_START   = 2048;
	static const int ALLY_VAR_START   = 3072;
	static const int GLOBAL_VAR_START = 4096;

	static const int UNIT_VAR_END   = UNIT_VAR_START   + UNIT_VAR_COUNT   - 1;
	static const int TEAM_VAR_END   = TEAM_VAR_START   + TEAM_VAR_COUNT   - 1;
	static const int ALLY_VAR_END   = ALLY_VAR_START   + ALLY_VAR_COUNT   - 1;
	static const int GLOBAL_VAR_END = GLOBAL_VAR_START + GLOBAL_VAR_COUNT - 1;

	static void InitVars(int numTeams, int numAllyTeams);

protected:
	CCobFile& script;
	bool yardOpen;

	static std::vector<int> teamVars[TEAM_VAR_COUNT];
	static std::vector<int> allyVars[ALLY_VAR_COUNT];
	static int globalVars[GLOBAL_VAR_COUNT];

	int unitVars[UNIT_VAR_COUNT];

	virtual int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	virtual void ShowScriptError(const std::string& msg);
	virtual void ShowScriptWarning(const std::string& msg);

public:
	bool busy;
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

	// used by CCobThread
	void Signal(int signal);
	int GetUnitVal(int val, int p1, int p2, int p3, int p4);
	void SetUnitVal(int val, int param);
	void PlayUnitSound(int snr, int attr);
};

#endif // __COB_INSTANCE_H__
