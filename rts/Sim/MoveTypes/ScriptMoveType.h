/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCRIPT_MOVE_TYPE_H
#define SCRIPT_MOVE_TYPE_H

#include "MoveType.h"

class CScriptMoveType : public AMoveType
{
	CR_DECLARE(CScriptMoveType)

	public:
		CScriptMoveType(CUnit* owner);
		virtual ~CScriptMoveType();

	public:
		bool Update();
		void ForceUpdates();

		void SetPhysics(const float3& pos, const float3& vel, const float3& rot);
		void SetPosition(const float3& pos);
		void SetVelocity(const float3& vel);
		void SetRelativeVelocity(const float3& rvel);
		void SetRotation(const float3& rot);
		void SetRotationVelocity(const float3& rvel);
		void SetHeading(short heading);
		void SetNoBlocking(bool state);

	public: // null'ed virtuals
		void StartMoving(float3, float goalRadius) {}
		void StartMoving(float3, float goalRadius, float speed) {}
		void KeepPointingTo(float3, float distance, bool aggressive) {}
		void KeepPointingTo(CUnit* unit, float distance, bool aggressive) {}
		void StopMoving(bool callScript = false, bool hardStop = false) {}

		void SetGoal(const float3& pos, float distance = 0.0f) {}
		void SetMaxSpeed(float speed) {}
		void SetWantedMaxSpeed(float speed) {}
		void LeaveTransport() {}

	protected:
		void CheckLimits();
		void CheckNotify();

	public:
		int tag;

		bool extrapolate;
		bool useRelVel;
		bool useRotVel;

		float drag;

		/// velocity vector
		float3 velVec;
		/// relative velocity (to current direction)
		float3 relVel;

		/// angular position
		float3 rot;
		/// angular velocity
		float3 rotVel;

		float3 mins;
		float3 maxs;

		bool trackSlope;
		bool trackGround;
		float groundOffset;

		float gravityFactor;
		float windFactor;

		bool noBlocking;

		bool gndStop;
		bool shotStop;
		bool slopeStop;
		bool collideStop;

	protected:
		int scriptNotify;
};

#endif // SCRIPT_MOVE_TYPE_H
