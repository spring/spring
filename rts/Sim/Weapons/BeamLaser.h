/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAMLASER_H
#define BEAMLASER_H

#include "Weapon.h"

class CBeamLaser: public CWeapon
{
	CR_DECLARE_DERIVED(CBeamLaser)
	CR_DECLARE_SUB(SweepFireState)

public:
	CBeamLaser(CUnit* owner = nullptr, const WeaponDef* def = nullptr);

	void Update() override final;
	void Init() override final;

private:
	float3 GetFireDir(bool sweepFire, bool scriptCall);

	void UpdatePosAndMuzzlePos();
	float GetPredictedImpactTime(float3 p) const override final;
	void UpdateSweep();

	void FireInternal(float3 curDir);
	void FireImpl(const bool scriptCall) override final;

private:
	float3 color;
	float3 oldDir;

	float salvoDamageMult;

	struct SweepFireState {
	public:
		CR_DECLARE_STRUCT(SweepFireState)
	
		SweepFireState() {
			sweepInitDst = 0.0f;
			sweepGoalDst = 0.0f;
			sweepCurrDst = 0.0f;
			sweepStartAngle = 0.0f;

			sweepFiring = false;
			damageAllies = false;
		}

		void Init(const float3& newTargetPos, const float3& muzzlePos);
		void Update(const float3& newTargetDir) {
			sweepCurrDir = newTargetDir;
			sweepCurrDst = sweepCurrDir.LengthNormalize();
		}

		void SetSweepTempDir(const float3& dir) { sweepTempDir = dir; }
		void SetSweepCurrDir(const float3& dir) { sweepCurrDir = dir; }
		const float3& GetSweepTempDir() const { return sweepTempDir; } 
		const float3& GetSweepCurrDir() const { return sweepCurrDir; }

		float GetTargetDist2D() const;
		float GetTargetDist3D() const { return sweepCurrDst; }

		bool StartSweep(const float3& newTargetPos) const { return (newTargetPos != sweepGoalPos); }
		bool StopSweep() const { return (sweepFiring && (sweepCurrDir.dot(sweepGoalDir) >= 0.95f)); }

		void SetSweepFiring(bool b) { sweepFiring = b; }
		void SetDamageAllies(bool b) { damageAllies = b; }
		bool IsSweepFiring() const { return sweepFiring; }
		bool DamageAllies() const { return damageAllies; }

	private:
		float3 sweepInitPos; // 3D
		float3 sweepGoalPos; // 3D
		float3 sweepInitDir; // 3D
		float3 sweepGoalDir; // 3D
		float3 sweepCurrDir; // 3D
		float3 sweepTempDir; // 3D

		float sweepInitDst; // 2D
		float sweepGoalDst; // 2D
		float sweepCurrDst; // 3D (!)
		float sweepStartAngle; // radians

		bool sweepFiring;
		bool damageAllies;
	};

	SweepFireState sweepFireState;
};

#endif
