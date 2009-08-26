/* Author: Tobi Vollebregt */

#ifndef LUASCRIPTNAMES_H
#define LUASCRIPTNAMES_H


#include <map>
#include <string>
#include <vector>


// These are indices into an array of 'refs' (lua_ref / lua_unref)
// maintained by each CLuaUnitScript.

// All call-ins also get as first argument the unitID.
// This is left out of the comments below for brevity.

enum {
	LUAFN_Destroy,              // ( ) -> nil
	LUAFN_StartMoving,          // ( ) -> nil
	LUAFN_StopMoving,           // ( ) -> nil
	LUAFN_Activate,             // ( ) -> nil
	LUAFN_Killed,               // ( recentDamage, maxHealth ) -> number delayedWreckLevel | nil
	LUAFN_Deactivate,           // ( ) -> nil
	LUAFN_WindChanged,          // ( heading, strength ) -> nil
	LUAFN_ExtractionRateChanged,// ( newRate ) -> nil
	LUAFN_RockUnit,             // ( rockDir_x, rockDir_z ) -> nil
	LUAFN_HitByWeapon,          // ( hitDir_x, hitDir_z ) -> nil
	LUAFN_MoveRate0,            // ( ) -> nil
	LUAFN_MoveRate1,            // ( ) -> nil
	LUAFN_MoveRate2,            // ( ) -> nil
	LUAFN_MoveRate3,            // FIXME: unused (see CTAAirMoveType::UpdateMoveRate)
	LUAFN_SetSFXOccupy,         // ( curTerrainType ) -> nil
	LUAFN_HitByWeaponId,        // ( hitDir_x, hitDir_z, weaponDefID, damage ) -> number newDamage
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
	LUAFN_Last,

	// These are special (this set of functions is repeated MAX_WEAPONS_PER_UNIT times)
	LUAFN_QueryPrimary = LUAFN_Last, // ( ) -> number piece
	LUAFN_AimPrimary,                // ( heading - owner->heading,  pitch ) -> nil   (both args 0 for plasma repulser)
	LUAFN_AimFromPrimary,            // ( ) -> number piece
	LUAFN_FirePrimary,               // ( ) -> nil
	LUAFN_EndBurst,                  // ( ) -> nil
	LUAFN_Shot,                      // ( ) -> nil
	LUAFN_BlockShot,                 // ( targetUnitID, haveUserTarget ) -> boolean
	LUAFN_TargetWeight,              // ( targetUnitID ) -> number targetWeight
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
