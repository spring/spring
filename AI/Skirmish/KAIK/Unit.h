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
		float Health();
		int category();

		const UnitDef* def() const;
		int myid;
		// 0: mine, 1: allied, 2: enemy -1: non-existant
		int owner();

		// the attackgroup that the unit might belong to
		// (added to support some AttackHandler/group features)
		int groupID;

		bool CanAttack(int otherUnit);
		bool CanAttackMe(int otherUnit);

		// used in groups when units don't respond to move orders
		int stuckCounter;
		float3 earlierPosition;
		// used in groups to microcontrol units to their max range of the chosen target
		int maneuverCounter;

		// specialized
		bool ReclaimBestFeature(bool metal, float radius = 1000);

		// construction
		int GetBestBuildFacing(float3& pos) const;
		bool Build_ClosestSite(const UnitDef* unitdef, float3 targetpos, int separation = DEFCBS_SEPARATION, float radius = DEFCBS_RADIUS);
		bool FactoryBuild(const UnitDef* built) const;

		// nuke- and hub-related functions
		bool NukeSiloBuild(void);
		bool isHub(void) const;
		bool HubBuild(const UnitDef* built) const;


		// target-based abilities
		bool Attack(int target);
		bool Capture(int target);
		bool Guard(int target);
		bool Load(int target);
		bool Reclaim(int target);
		bool Repair(int target);
		bool Ressurect(int target);
		bool Upgrade(int target, const UnitDef*);

		// location point abilities
		bool Build(float3 pos, const UnitDef* unit, int facing);
		bool BuildShift(float3 pos, const UnitDef* unit, int facing);
		bool Move(float3 pos);
		bool MoveShift(float3 pos);
		bool Patrol(float3 pos) const;
		bool PatrolShift(float3 pos);

		// radius abilities
		bool Attack(float3 pos, float radius);
		bool Ressurect(float3 pos, float radius);
		bool Reclaim(float3 pos, float radius);
		bool Capture(float3 pos, float radius);
		bool Restore(float3 pos, float radius);
		bool Load(float3 pos, float radius);
		bool Unload(float3 pos, float radius);

		// toggable abilities
		bool Cloaking(bool on);
		bool OnOff(bool on);

		// special abilities
		bool SelfDestruct();
		bool SetFireState(int state);
		bool Stop();
		bool SetMaxSpeed(float speed);



	private:
		AIClasses* ai;

		// command generators
		Command MakePosCommand(int cmdID, float3 pos, float radius = -1.0f, int facing = -1) const;
		Command MakeIntCommand(int cmdID, int param, int maxParam = 4999);
};


#endif
