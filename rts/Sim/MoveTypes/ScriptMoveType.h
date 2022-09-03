/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCRIPT_MOVE_TYPE_H
#define SCRIPT_MOVE_TYPE_H

#include "MoveType.h"

class CScriptMoveType : public AMoveType
{
	CR_DECLARE_DERIVED(CScriptMoveType)

	public:
		CScriptMoveType(CUnit* owner);
		virtual ~CScriptMoveType();

		void* GetPreallocContainer() { return owner; }  // creg

	public:
		bool Update() override;
		void ForceUpdates();

		void SetPhysics(const float3& pos, const float3& vel, const float3& rot);
		void SetPosition(const float3& pos);
		void SetVelocity(const float3& vel);
		void SetRelativeVelocity(const float3& rvel);
		void SetRotation(const float3& rot);
		void SetRotationVelocity(const float3& rvel);
		void SetHeading(short heading);
		void SetNoBlocking(bool state);

		enum ScriptNotifyState {
			HitNothing = 0,
			HitGround = 1,
			HitLimit = 2,
		};

	public: // null'ed virtuals
		void StartMoving(float3, float goalRadius) override {}
		void StartMoving(float3, float goalRadius, float speed) override {}
		void KeepPointingTo(float3, float distance, bool aggressive) override {}
		void KeepPointingTo(CUnit* unit, float distance, bool aggressive) override {}
		void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override {}

		void SetGoal(const float3& pos, float distance = 0.0f) override {}
		void SetMaxSpeed(float speed) override {}
		void SetWantedMaxSpeed(float speed) override {}
		void LeaveTransport() override {}

	protected:
		void CheckLimits();
		void CheckNotify();

	public:
		/// velocity vector
		float3 velVec;
		/// relative velocity (to current direction)
		float3 relVel;

		/// angular position
		float3 rot;
		/// angular velocity
		float3 rotVel;

		float3 mins = {-1.0e9f, -1.0e9f, -1.0e9f};
		float3 maxs = {+1.0e9f, +1.0e9f, +1.0e9f};

		int tag = 0;

		float drag = 0.0f;
		float groundOffset = 0.0f;

		float gravityFactor = 0.0f;
		float windFactor = 0.0f;

		bool extrapolate = true;
		bool useRelVel = false;
		bool useRotVel = false;

		bool trackSlope = false;
		bool trackGround = false;
		bool trackLimits = false;

		bool noBlocking = false;

		bool groundStop = false;
		bool limitsStop = false;

	protected:
		ScriptNotifyState scriptNotify = HitNothing;
};

#endif // SCRIPT_MOVE_TYPE_H
