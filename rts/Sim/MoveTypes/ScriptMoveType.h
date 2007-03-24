#ifndef SCRIPT_MOVE_TYPE_H
#define SCRIPT_MOVE_TYPE_H

#include "MoveType.h"

class CScriptMoveType : public CMoveType
{
	CR_DECLARE(CScriptMoveType);

	public:
		CScriptMoveType(CUnit* owner);
		virtual ~CScriptMoveType(void);

	public:
		void Update();
		void SlowUpdate();
		void ForceUpdates();

		void SetPhysics(const float3& pos, const float3& vel, const float3& rot);
		void SetPosition(const float3& pos);
		void SetVelocity(const float3& vel);
		void SetRelativeVelocity(const float3& rvel);
		void SetRotation(const float3& rot);
		void SetRotationVelocity(const float3& rvel);
		void SetRotationOffset(const float3& rotOff);
		void SetHeading(short heading);
		
	public: // null'ed virtuals  (FIXME -- redirect to the real MoveType?)
		void StartMoving(float3 pos, float goalRadius) {};
		void StartMoving(float3 pos, float goalRadius, float speed) {};
		void KeepPointingTo(float3 pos, float distance, bool aggressive) {};
		void KeepPointingTo(CUnit* unit, float distance, bool aggressive) {};
		void StopMoving() {};
		void Idle(unsigned int frames) {};
		void Idle() {};
		void DeIdle() {};
		void ImpulseAdded() {};
		void SetGoal(float3 pos) {};
		void SetMaxSpeed(float speed) {};
		void SetWantedMaxSpeed(float speed) {};
		void LeaveTransport(void) {};

	protected:
		void CalcMidPos();
		void CalcDirections();
		void TrackSlope();

	public:
		int tag;

		bool extrapolate;

		float3 vel;     // velocity
		float3 relVel;  // relative velocity (to current direction)
		bool useRelVel;

		float3 rot;    // angular position
		float3 rotVel; // angular velocity
		bool useRotVel;

		bool trackSlope;
		bool trackGround;
		float groundOffset;

		float gravityFactor;
		float windFactor;

		float3 mins;
		float3 maxs;

		bool shotStop;
		bool slopeStop;
		bool collideStop;

	protected:
		bool hasDecal;
		bool isBuilding;
		bool isBlocking;

		float3 rotOffset;		

		int lastTrackUpdate;
		float3 oldPos;
		float3 oldSlowUpdatePos;
};


#endif /* SCRIPT_MOVE_TYPE_H */
