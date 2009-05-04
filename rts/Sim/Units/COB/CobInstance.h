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

	void MapScriptToModelPieces(LocalModel* lmodel);

	virtual int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	virtual void ShowScriptError(const std::string& msg);
	virtual void ShowScriptWarning(const std::string& msg);

public:
	std::vector<int> staticVars;
	std::list<CCobThread *> threads;
	bool smoothAnim;
	const CCobFile* GetScriptAddr() const { return &script; }

public:
	CCobInstance(CCobFile &script, CUnit *unit);
	~CCobInstance();

	virtual int GetFunctionId(const std::string& fname) const;

	// used by CCobThread
	void Signal(int signal);
	void PlayUnitSound(int snr, int attr);

	// translate cob piece coords into worldcoordinates
	void Spin(int piece, int axis, int speed, int accel) {
		CUnitScript::Spin(piece, axis, speed * TAANG2RAD, accel * TAANG2RAD);
	}
	void StopSpin(int piece, int axis, int decel) {
		CUnitScript::StopSpin(piece, axis, decel * TAANG2RAD);
	}
	void Turn(int piece, int axis, int speed, int destination, bool interpolated = false) {
		CUnitScript::Turn(piece, axis, speed * TAANG2RAD, destination * TAANG2RAD, interpolated);
	}
	void Move(int piece, int axis, int speed, int destination, bool interpolated = false) {
		CUnitScript::Move(piece, axis, speed * CORDDIV, destination * CORDDIV, interpolated);
	}
	void MoveNow(int piece, int axis, int destination) {
		CUnitScript::MoveNow(piece, axis, destination * CORDDIV);
	}
	void TurnNow(int piece, int axis, int destination) {
		CUnitScript::TurnNow(piece, axis, destination * TAANG2RAD);
	}
	void MoveSmooth(int piece, int axis, int destination, int delta, int deltaTime) {
		CUnitScript::MoveSmooth(piece, axis, destination * CORDDIV, delta, deltaTime);
	}
	void TurnSmooth(int piece, int axis, int destination, int delta, int deltaTime) {
		CUnitScript::TurnSmooth(piece, axis, destination * TAANG2RAD, delta, deltaTime);
	}
};

#endif // __COB_INSTANCE_H__
