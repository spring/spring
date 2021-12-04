/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COBSCRIPTNAMES_H
#define COBSCRIPTNAMES_H

#include <string>
#include <array>

#include "Sim/Misc/GlobalConstants.h"
#include "System/UnorderedMap.hpp"


//These are mapped by the CCobFile at startup to make common function calls faster
enum {
	COBFN_Create,               // -
	COBFN_Destroy,              // -
	COBFN_StartMoving,          // in: reversing
	COBFN_StopMoving,           // -
	COBFN_Activate,             // -
	COBFN_Killed,               // in: recentDamage / maxHealth * 100, out: delayedWreckLevel
	COBFN_Deactivate,           // -
	COBFN_SetDirection,         // in: heading
	COBFN_SetSpeed,             // in: windStrength * 3000  OR  in: metalExtract * 500
	COBFN_RockUnit,             // in: 500 * rockDir.z, in: 500 * rockDir.x
	COBFN_HitByWeapon,          // in: 500 * hitDir.z, in: 500 * hitDir.x
	COBFN_MoveRate0,            // -
	COBFN_MoveRate1,            // -
	COBFN_MoveRate2,            // -
	COBFN_MoveRate3,            // FIXME: unused (see CHoverAirMoveType::UpdateMoveRate)
	COBFN_SetSFXOccupy,         // in: curTerrainType
	COBFN_HitByWeaponId,        // in: 500 * hitDir.z, in: 500 * hitDir.x, in: weaponDefs[weaponId].tdfId, in: 100 * damage, return value: 100 * weaponHitMod
	COBFN_QueryLandingPadCount, // out: landingPadCount (default 16)
	COBFN_QueryLandingPad,      // landingPadCount times (out: piecenum)
	COBFN_Falling,              // -
	COBFN_Landed,               // -
	COBFN_BeginTransport,       // in: unit->model->height*65536
	COBFN_QueryTransport,       // out: piecenum, in: unit->model->height*65536
	COBFN_TransportPickup,      // in: unit->id
	COBFN_StartUnload,          // -
	COBFN_EndTransport,         // -
	COBFN_TransportDrop,        // in: unit->id, in: PACKXZ(pos.x, pos.z)
	COBFN_SetMaxReloadTime,     // in: maxReloadTime
	COBFN_StartBuilding,        // BUILDER: in: h-heading, in: p-pitch; FACTORY: -
	COBFN_StopBuilding,         // -
	COBFN_QueryNanoPiece,       // out: piecenum
	COBFN_QueryBuildInfo,       // out: piecenum
	COBFN_Go,                   // -
	COBFN_Last,

	// These are special (this set of functions is repeated MAX_WEAPONS_PER_UNIT times)
	COBFN_QueryPrimary = COBFN_Last, // out: piecenum
	COBFN_AimPrimary,                // in: heading - owner->heading, in: pitch (both 0 for plasma repulser)
	COBFN_AimFromPrimary,            // out: piecenum
	COBFN_FirePrimary,               // -
	COBFN_EndBurst,                  // -
	COBFN_Shot,                      // in: 0
	COBFN_BlockShot,                 // in: targetUnit->id or 0, out: blockShot, in: haveUserTarget
	COBFN_TargetWeight,              // in: targetUnit->id or 0, out: targetWeight*65536

	COBFN_Weapon_Last,
	COBFN_Weapon_Funcs = COBFN_Weapon_Last - COBFN_Last,
	COBFN_NumUnitFuncs = COBFN_Last + (MAX_WEAPONS_PER_UNIT * COBFN_Weapon_Funcs),
};


class CCobUnitScriptNames
{
public:
	static void InitScriptNames();

	static const std::array<std::string, COBFN_NumUnitFuncs>& GetScriptNames(); // COBFN_* -> string
	static const spring::unordered_map<std::string, int>& GetScriptMap(); // string -> COBFN_*

	static int GetScriptNumber(const std::string& fname);
	static const std::string& GetScriptName(unsigned int num);
};

#endif
