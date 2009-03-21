#ifndef UNIT_H
#define UNIT_H

#include "System/StdAfx.h"
#include "creg/creg.h"
#include "Sim/Units/UnitDef.h"				// Unit Definitions

#include "Defines.h"

class AIClasses;


class CUNIT {
	public:
		CR_DECLARE(CUNIT);

		CUNIT(void);
		CUNIT(AIClasses* ai);
		~CUNIT();
		void PostLoad();

		// misc. info
		float3 pos() const;
		float Health() const;
		int category() const;

		const UnitDef* def() const;
		int myid;
		// 0: mine, 1: allied, 2: enemy -1: non-existant
		int owner() const;

		// the attackgroup that the unit might belong to
		// (added to support some AttackHandler/group features)
		int groupID;

		bool CanAttack(int otherUnit) const;
		bool CanAttackMe(int otherUnit) const;

		// used in groups when units don't respond to move orders
		int stuckCounter;
		float3 earlierPosition;
		// used in groups to microcontrol units to their max range of the chosen target
		int maneuverCounter;

		// specialized
		bool ReclaimBestFeature(bool metal, float radius = 1000);

		// construction
		int GetBestBuildFacing(const float3& pos) const;
		bool Build_ClosestSite(const UnitDef* def, const float3& pos, int separation = DEFCBS_SEPARATION, float radius = DEFCBS_RADIUS);
		bool FactoryBuild(const UnitDef* built) const;

		// nuke- and hub-related functions
		bool NukeSiloBuild(void) const;
		bool isHub(void) const;
		bool HubBuild(const UnitDef* built) const;


		// target-based abilities
		bool Attack(int target) const;
		bool Capture(int target) const;
		bool Guard(int target) const;
		bool Load(int target) const;
		bool Reclaim(int target) const;
		bool Repair(int target) const;
		bool Ressurect(int target) const;
		bool Upgrade(int target, const UnitDef*) const;

		// location point abilities
		bool Build(float3 pos, const UnitDef* unit, int facing) const;
		bool BuildShift(float3 pos, const UnitDef* unit, int facing) const;
		bool Move(float3 pos) const;
		bool MoveShift(float3 pos) const;
		bool Patrol(float3 pos) const;
		bool PatrolShift(float3 pos) const;

		// radius abilities
		bool Attack(float3 pos, float radius) const;
		bool Ressurect(float3 pos, float radius) const;
		bool Reclaim(float3 pos, float radius) const;
		bool Capture(float3 pos, float radius) const;
		bool Restore(float3 pos, float radius) const;
		bool Load(float3 pos, float radius) const;
		bool Unload(float3 pos, float radius) const;

		// toggable abilities
		bool Cloaking(bool on) const;
		bool OnOff(bool on) const;

		// special abilities
		bool SelfDestruct() const;
		bool SetFireState(int state) const;
		bool Stop() const;
		bool SetMaxSpeed(float speed) const;

	private:
		AIClasses* ai;

		// command generators
		Command MakePosCommand(int cmdID, float3 pos, float radius = -1.0f, int facing = -1) const;
		Command MakeIntCommand(int cmdID, int param) const;
};


#endif
