/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCRIPT_MOVE_TYPE_H
#define SCRIPT_MOVE_TYPE_H

#include "MoveType.h"

class CScriptMoveType : public AMoveType
{
	CR_DECLARE(CScriptMoveType);

	public:
		CScriptMoveType(CUnit* owner);
		virtual ~CScriptMoveType(void);

	public:
		void Update();
		void ForceUpdates();

		void SetPhysics(const float3& pos, const float3& vel, const float3& rot);
		void SetPosition(const float3& pos);
		void SetVelocity(const float3& vel);
		void SetRelativeVelocity(const float3& rvel);
		void SetRotation(const float3& rot);
		void SetRotationVelocity(const float3& rvel);
		void SetRotationOffset(const float3& rotOff);
		void SetHeading(short heading);
		void SetNoBlocking(bool state);
		
	public: // null'ed virtuals
		void StartMoving(float3, float goalRadius) {}
		void StartMoving(float3, float goalRadius, float speed) {}
		void KeepPointingTo(float3, float distance, bool aggressive) {}
		void KeepPointingTo(CUnit* unit, float distance, bool aggressive) {}
		void StopMoving() {}
		void Idle(unsigned int frames) {}
		void Idle() {}
		void DeIdle() {}
		void ImpulseAdded() {}
		void SetGoal(float3 pos) {}
		void SetMaxSpeed(float speed) {}
		void SetWantedMaxSpeed(float speed) {}
		void LeaveTransport(void) {}

	protected:
		void CalcDirections();
		void TrackSlope();
		void CheckLimits();
		void CheckNotify();

	public:
		int tag;
		
		bool extrapolate;

		float drag;

		/// velocity
		float3 vel;
		/// relative velocity (to current direction)
		float3 relVel;
		bool useRelVel;

		/// angular position
		float3 rot;
		/// angular velocity
		float3 rotVel;
		bool useRotVel;

		bool trackSlope;
		bool trackGround;
		float groundOffset;

		float gravityFactor;
		float windFactor;

		float3 mins;
		float3 maxs;

		bool noBlocking;

		bool gndStop;
		bool shotStop;
		bool slopeStop;
		bool collideStop;

		bool leaveTracks;

	protected:
		float3 rotOffset;

		int lastTrackUpdate;
		int scriptNotify;
};

#endif // SCRIPT_MOVE_TYPE_H
