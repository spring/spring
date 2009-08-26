/* Author: Tobi Vollebregt */

#ifndef LUASCRIPTNAMES_H
#define LUASCRIPTNAMES_H


#include <map>
#include <string>
#include <vector>


//These are mapped by the CCobFile at startup to make common function calls faster
// TODO: check scales of non-ratio arguments
enum {
	LUAFN_Create,               // -
	LUAFN_Destroy,              // -
	LUAFN_StartMoving,          // -
	LUAFN_StopMoving,           // -
	LUAFN_Activate,             // -
	LUAFN_Killed,               // in: recentDamage / maxHealth * 100, out: delayedWreckLevel
	LUAFN_Deactivate,           // -
	LUAFN_SetDirection,         // in: heading
	LUAFN_SetSpeed,             // in: windStrength * 3000  OR  in: metalExtract * 500
	LUAFN_RockUnit,             // in: 500 * rockDir.z, in: 500 * rockDir.x
	LUAFN_HitByWeapon,          // in: 500 * hitDir.z, in: 500 * hitDir.x
	LUAFN_MoveRate0,            // -
	LUAFN_MoveRate1,            // -
	LUAFN_MoveRate2,            // -
	LUAFN_MoveRate3,            // FIXME: unused (see CTAAirMoveType::UpdateMoveRate)
	LUAFN_SetSFXOccupy,         // in: curTerrainType
	LUAFN_HitByWeaponId,        // in: 500 * hitDir.z, in: 500 * hitDir.x, in: weaponDefs[weaponId].tdfId, in: 100 * damage, return value: 100 * weaponHitMod
	LUAFN_QueryLandingPadCount, // out: landingPadCount (default 16)
	LUAFN_QueryLandingPad,      // landingPadCount times (out: piecenum)
	LUAFN_Falling,              // -
	LUAFN_Landed,               // -
	LUAFN_BeginTransport,       // in: unit->model->height*65536
	LUAFN_QueryTransport,       // out: piecenum, in: unit->model->height*65536
	LUAFN_TransportPickup,      // in: unit->id
	LUAFN_StartUnload,          // -
	LUAFN_EndTransport,         // -
	LUAFN_TransportDrop,        // in: unit->id, in: PACKXZ(pos.x, pos.z)
	LUAFN_SetMaxReloadTime,     // in: maxReloadTime
	LUAFN_StartBuilding,        // BUILDER: in: h-heading, in: p-pitch; FACTORY: -
	LUAFN_StopBuilding,         // -
	LUAFN_QueryNanoPiece,       // out: piecenum
	LUAFN_QueryBuildInfo,       // out: piecenum
	LUAFN_Go,                   // -
	LUAFN_MoveFinished,         // special callin, only used for Lua unit scripts
	LUAFN_TurnFinished,         // idem
	LUAFN_Last,

	// These are special (this set of functions is repeated MAX_WEAPONS_PER_UNIT times)
	LUAFN_QueryPrimary = LUAFN_Last, // out: piecenum
	LUAFN_AimPrimary,                // in: heading - owner->heading, in: pitch (both 0 for plasma repulser)
	LUAFN_AimFromPrimary,            // out: piecenum
	LUAFN_FirePrimary,               // -
	LUAFN_EndBurst,                  // -
	LUAFN_Shot,                      // in: 0
	LUAFN_BlockShot,                 // in: targetUnit->id or 0, out: blockShot, in: haveUserTarget
	LUAFN_TargetWeight,              // in: targetUnit->id or 0, out: targetWeight*65536
	LUAFN_Weapon_Last,
	LUAFN_Weapon_Funcs = LUAFN_Weapon_Last - LUAFN_Last,
};


class CLuaUnitScriptNames
{
public:
	static const std::vector<std::string>& GetScriptNames(); // LUAFN_* -> string
	static const std::map<std::string, int>& GetScriptMap(); // string -> LUAFN_*

	static int GetScriptNumber(const std::string& fname);
	static const std::string& GetScriptName(int num);
};

#endif
