/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUASCRIPTNAMES_H
#define LUASCRIPTNAMES_H

#include <string>
#include <array>

#include "System/UnorderedMap.hpp"


// These are indices into an array of 'refs' (lua_ref / lua_unref)
// maintained by each CLuaUnitScript.

enum {
	LUAFN_Destroy,              // ( ) -> nil
	LUAFN_StartMoving,          // ( reversing ) -> nil
	LUAFN_StopMoving,           // ( ) -> nil
	LUAFN_StartSkidding,        // ( vx, vy, vz) -> nil
	LUAFN_StopSkidding,         // ( ) -> nil
	LUAFN_ChangeHeading,        // ( deltaHeading ) -> nil
	LUAFN_Activate,             // ( ) -> nil
	LUAFN_Killed,               // ( recentDamage, maxHealth ) -> number delayedWreckLevel | nil
	LUAFN_Deactivate,           // ( ) -> nil
	LUAFN_WindChanged,          // ( heading, strength ) -> nil
	LUAFN_ExtractionRateChanged,// ( newRate ) -> nil
	LUAFN_RockUnit,             // ( rockDir_x, rockDir_z ) -> nil
	LUAFN_MoveRate,             // ( curMoveRate ) -> nil
	LUAFN_SetSFXOccupy,         // ( curTerrainType ) -> nil
	LUAFN_HitByWeapon,          // ( hitDir_x, hitDir_z, weaponDefID, damage ) -> number newDamage | nil
	LUAFN_QueryLandingPads,     // ( ) -> table piecenums
	LUAFN_Falling,              // ( ) -> nil
	LUAFN_Landed,               // ( ) -> nil
	LUAFN_BeginTransport,       // ( passengerID ) -> nil
	LUAFN_QueryTransport,       // ( passengerID ) -> number piece
	LUAFN_TransportPickup,      // ( passengerID ) -> nil
	LUAFN_StartUnload,          // ( ) -> nil
	LUAFN_EndTransport,         // ( ) -> nil
	LUAFN_TransportDrop,        // ( passengerID, x, y, z ) -> nil
	LUAFN_StartBuilding,        // BUILDER: ( h-heading, p-pitch ) -> nil ; FACTORY: ( ) -> nil
	LUAFN_StopBuilding,         // ( ) -> nil
	LUAFN_QueryNanoPiece,       // ( ) -> number piece
	LUAFN_QueryBuildInfo,       // ( ) -> number piece
	LUAFN_MoveFinished,         // ( piece, axis ) -> nil
	LUAFN_TurnFinished,         // ( piece, axis ) -> nil

	// Weapon functions
	LUAFN_QueryWeapon,   // ( ) -> number piece
	LUAFN_AimWeapon,     // ( heading - owner->heading,  pitch ) -> nil
	LUAFN_AimShield,     // ( ) -> nil
	LUAFN_AimFromWeapon, // ( ) -> number piece
	LUAFN_FireWeapon,    // ( ) -> nil
	LUAFN_EndBurst,      // ( ) -> nil
	LUAFN_Shot,          // ( ) -> nil
	LUAFN_BlockShot,     // ( targetUnitID, haveUserTarget ) -> boolean
	LUAFN_TargetWeight,  // ( targetUnitID ) -> number targetWeight

	LUAFN_Last,
};


class CLuaUnitScriptNames
{
public:
	static void InitScriptNames();

	static const std::array<std::string, LUAFN_Last>& GetScriptNames(); // LUAFN_* -> string
	static const spring::unordered_map<std::string, int>& GetScriptMap(); // string -> LUAFN_*

	static int GetScriptNumber(const std::string& fname);
	static const std::string& GetScriptName(unsigned int num);
};

#endif
