/* Author: Tobi Vollebregt */
/* heavily based on CobInstance.h */

#ifndef UNITSCRIPT_H
#define UNITSCRIPT_H

#include <string>
#include <vector>
#include <list>

#include "Object.h"
#include "Rendering/UnitModels/3DModel.h"


static const int COBSCALE = 65536;
static const int COBSCALEHALF = COBSCALE / 2;
static const float CORDDIV   = 1.0f / COBSCALE;
static const float RAD2TAANG = COBSCALEHALF / PI;
static const float TAANG2RAD = PI / COBSCALEHALF;


class CUnit;

typedef void (*CBCobThreadFinish) (int retCode, void *p1, void *p2);


class CUnitScript : public CObject
{
public:
	enum AnimType {ATurn, ASpin, AMove};

	struct IAnimListener {
		virtual ~IAnimListener() = 0;
		virtual void AnimFinished(AnimType type, int piece, int axis) = 0;
	};

protected:
	struct AnimInfo {
		AnimType type;
		int axis;
		int piece;
		float speed;
		float dest;		//means final position when turning or moving, final speed when spinning
		float accel;		//used for spinning, can be negative
		bool interpolated;	//true if this animation is a result of interpolating a direct move/turn
		std::list<IAnimListener *> listeners;
	};

	std::list<struct AnimInfo *> anims;
	CUnit *unit;
	const std::vector<int>& scriptIndex;

	void MapScriptToModelPieces(LocalModel* lmodel);
	void UnblockAll(struct AnimInfo * anim);

public:
	std::vector<LocalModelPiece*> pieces;

	LocalModelPiece* GetLocalModelPiece(int scriptnum) const {
		if (scriptnum >= 0 && (size_t)scriptnum < pieces.size()) {
			return pieces[scriptnum];
		}else{
			return NULL;
		}
	};

	int ScriptToModel(int scriptnum) const {
		LocalModelPiece* p = GetLocalModelPiece(scriptnum);

		if (p == NULL) return -1;

		int i = 0;
		std::vector<LocalModelPiece*> *modelpieces = &unit->localmodel->pieces;
		for(std::vector<LocalModelPiece*>::iterator pm=modelpieces->begin();pm!=modelpieces->end();pm++,i++) {
			if (p == *pm) return i;
		}
		return -1;
	};

	bool PieceExists(int scriptnum) const {
		return GetLocalModelPiece(scriptnum) != NULL;
	};

#define SCRIPT_TO_LOCALPIECE_FUNC(x,y,z,w) \
	x y(int scriptnum) const { \
		LocalModelPiece* p = GetLocalModelPiece(scriptnum); \
		if (p != NULL) return p->z(); \
		return w; \
	};

	SCRIPT_TO_LOCALPIECE_FUNC(float3,     GetPiecePos,       GetPos,       float3(0.0f,0.0f,0.0f))
	SCRIPT_TO_LOCALPIECE_FUNC(CMatrix44f, GetPieceMatrix,    GetMatrix,    CMatrix44f())
	SCRIPT_TO_LOCALPIECE_FUNC(float3,     GetPieceDirection, GetDirection, float3(1.0f,1.0f,1.0f))
	//SCRIPT_TO_LOCALPIECE_FUNC(int,        GetPieceVertCount, GetVertCount, 0)

	bool GetEmitDirPos(int scriptnum, float3 &pos, float3 &dir) const {
		LocalModelPiece* p = GetLocalModelPiece(scriptnum);
		if (p != NULL) {
			return p->GetEmitDirPos(pos, dir);
		} else {
			return true;
		}
	};

public:
	CUnitScript(CUnit* unit, const std::vector<int>& scriptIndex);
	virtual ~CUnitScript();
	      CUnit* GetUnit()       { return unit; }
	const CUnit* GetUnit() const { return unit; }
	int Call(const std::string &fname);
	int Call(const std::string &fname, int p1);
	int Call(const std::string &fname, std::vector<int> &args);
	int Call(const std::string &fname, CBCobThreadFinish cb, void *p1, void *p2);
	int Call(const std::string &fname, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	int Call(int id);
	int Call(int id, std::vector<int> &args);
	int Call(int id, int p1);
	int Call(int id, CBCobThreadFinish cb, void *p1, void *p2);
	int Call(int id, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	int RawCall(int fn, std::vector<int> &args);
	int RawCall(int fn, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	virtual int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2) = 0;
	int Tick(int deltaTime);
	int MoveToward(float &cur, float dest, float speed);
	int TurnToward(float &cur, float dest, float speed);
	int DoSpin(float &cur, float dest, float &speed, float accel, int divisor);
	void Spin(int piece, int axis, int speed, int accel);
	void StopSpin(int piece, int axis, int decel);
	void Turn(int piece, int axis, int speed, int destination, bool interpolated = false);
	void Move(int piece, int axis, int speed, int destination, bool interpolated = false);
	void MoveNow(int piece, int axis, int destination);
	void TurnNow(int piece, int axis, int destination);
	void SetVisibility(int piece, bool visible);
	void EmitSfx(int type, int piece);
	void AttachUnit(int piece, int unit);
	void DropUnit(int unit);
	struct AnimInfo *FindAnim(AnimType anim, int piece, int axis);
	void RemoveAnim(AnimType anim, int piece, int axis);
	void AddAnim(AnimType type, int piece, int axis, int speed, int dest, int accel, bool interpolated = false);
	int AddTurnListener(int piece, int axis, IAnimListener *listener);
	int AddMoveListener(int piece, int axis, IAnimListener *listener);
	void Signal(int signal);
	int GetUnitVal(int val, int p1, int p2, int p3, int p4);
	void SetUnitVal(int val, int param);
	void Explode(int piece, int flags);
	void PlayUnitSound(int snr, int attr);
	void ShowFlare(int piece);
	void MoveSmooth(int piece, int axis, int destination, int delta, int deltaTime);
	void TurnSmooth(int piece, int axis, int destination, int delta, int deltaTime);
	bool HasScriptFunction(int id);
	bool FunctionExist(int id);
	int GetFunctionId(const std::string& funcName) const;
};

#endif
