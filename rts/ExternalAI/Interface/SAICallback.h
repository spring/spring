/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SAICALLBACK_H
#define	_SAICALLBACK_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "aidefines.h"

#include <stdbool.h>

/**
 * Skirmish AI Callback function pointers.
 */
struct SAICallback {

/**
 * Whenever an AI wants to change engine state in any way, it has to call this method.
 * In other words, all commands from AIs to the engine (and other AIs) go through this method.
 *
 * teamId	the team number of the AI that sends the command
 * toId		the team number of the AI that should receive the command,
 * 		or COMMAND_TO_ID_ENGINE if it is meant for the engine
 * commandId	used on asynchronous commands, this allows the AI to identify
 * 		a possible result event, which would come with the same id
 * commandTopic	unique identifyer of a command (see COMMAND_* defines in AISCommands.h)
 * commandData	a commandTopic specific struct, which contains the data associated
 * 		with the command (see *Command structs)
 * return	0 if command handling ok, something else otherwise
 */
int (CALLING_CONV *handleCommand)(int teamId, int toId, int commandId, int commandTopic, void* commandData);


// BEGINN misc callback functions
int (CALLING_CONV *Game_getCurrentFrame)(int teamId); //TODO: deprecate, becuase we get the frame from the SUpdateEvent
int (CALLING_CONV *Game_getAiInterfaceVersion)(int teamId);
int (CALLING_CONV *Game_getMyTeam)(int teamId);
int (CALLING_CONV *Game_getMyAllyTeam)(int teamId);
int (CALLING_CONV *Game_getPlayerTeam)(int teamId, int playerId);
const char* (CALLING_CONV *Game_getTeamSide)(int teamId, int otherTeamId);
bool (CALLING_CONV *Game_isExceptionHandlingEnabled)(int teamId);
bool (CALLING_CONV *Game_isDebugModeEnabled)(int teamId);
int (CALLING_CONV *Game_getMode)(int teamId);
bool (CALLING_CONV *Game_isPaused)(int teamId);
float (CALLING_CONV *Game_getSpeedFactor)(int teamId);
const char* (CALLING_CONV *Game_getSetupScript)(int teamId);
// END misc callback functions


// BEGINN Visualization related callback functions
float (CALLING_CONV *Gui_getViewRange)(int teamId);
float (CALLING_CONV *Gui_getScreenX)(int teamId);
float (CALLING_CONV *Gui_getScreenY)(int teamId);
struct SAIFloat3 (CALLING_CONV *Gui_Camera_getDirection)(int teamId);
struct SAIFloat3 (CALLING_CONV *Gui_Camera_getPosition)(int teamId);
// END Visualization related callback functions


// BEGINN kind of deprecated; it is recommended not to use these
//bool (CALLING_CONV *getProperty)(int teamId, int id, int property, void* dst);
//bool (CALLING_CONV *getValue)(int teamId, int id, void* dst);
// END kind of deprecated; it is recommended not to use these


// BEGINN OBJECT Cheats
bool (CALLING_CONV *Cheats_isEnabled)(int teamId);
bool (CALLING_CONV *Cheats_setEnabled)(int teamId, bool enable);
bool (CALLING_CONV *Cheats_setEventsEnabled)(int teamId, bool enabled);
bool (CALLING_CONV *Cheats_isOnlyPassive)(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT ResourceInfo
float (CALLING_CONV *ResourceInfo_Metal_getCurrent)(int teamId);
float (CALLING_CONV *ResourceInfo_Metal_getIncome)(int teamId);
float (CALLING_CONV *ResourceInfo_Metal_getUsage)(int teamId);
float (CALLING_CONV *ResourceInfo_Metal_getStorage)(int teamId);
float (CALLING_CONV *ResourceInfo_Energy_getCurrent)(int teamId);
float (CALLING_CONV *ResourceInfo_Energy_getIncome)(int teamId);
float (CALLING_CONV *ResourceInfo_Energy_getUsage)(int teamId);
float (CALLING_CONV *ResourceInfo_Energy_getStorage)(int teamId);
// END OBJECT ResourceInfo


// BEGINN OBJECT File
int (CALLING_CONV *File_getSize)(int teamId, const char* fileName);
bool (CALLING_CONV *File_getContent)(int teamId, const char* filename, void* buffer, int bufferLen);
bool (CALLING_CONV *File_locateForReading)(int teamId, char* filename);
bool (CALLING_CONV *File_locateForWriting)(int teamId, char* filename);
// END OBJECT File


// BEGINN OBJECT UnitDef
int (CALLING_CONV *UnitDef_STATIC_getIds)(int teamId, int unitDefIds[]);
int (CALLING_CONV *UnitDef_STATIC_getNumIds)(int teamId);
int (CALLING_CONV *UnitDef_STATIC_getIdByName)(int teamId, const char* unitName);
float (CALLING_CONV *UnitDef_getHeight)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getRadius)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isValid)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getName)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getHumanName)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getFilename)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getId)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getAiHint)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getCobID)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getTechLevel)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getGaia)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMetalUpkeep)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getEnergyUpkeep)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMetalMake)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMakesMetal)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getEnergyMake)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMetalCost)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getEnergyCost)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getBuildTime)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getExtractsMetal)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getExtractRange)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getWindGenerator)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTidalGenerator)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMetalStorage)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getEnergyStorage)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getAutoHeal)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getIdleAutoHeal)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getIdleTime)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getPower)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getHealth)(int teamId, int unitDefId);
unsigned int (CALLING_CONV *UnitDef_getCategory)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTurnRate)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isTurnInPlace)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getMoveType)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isUpright)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCollide)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getControlRadius)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getLosRadius)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getAirLosRadius)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getLosHeight)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getRadarRadius)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getSonarRadius)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getJammerRadius)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getSonarJamRadius)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getSeismicRadius)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getSeismicSignature)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isStealth)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isSonarStealth)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isBuildRange3D)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getBuildDistance)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getBuildSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getReclaimSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getRepairSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxRepairSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getResurrectSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getCaptureSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTerraformSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMass)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isPushResistant)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isStrafeToAttack)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMinCollisionSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getSlideTolerance)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxSlope)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxHeightDif)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMinWaterDepth)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getWaterline)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxWaterDepth)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getArmoredMultiple)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getArmorType)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getFlankingBonusMode)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *UnitDef_getFlankingBonusDir)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFlankingBonusMax)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFlankingBonusMin)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFlankingBonusMobilityAdd)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getCollisionVolumeType)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *UnitDef_getCollisionVolumeScales)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *UnitDef_getCollisionVolumeOffsets)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getCollisionVolumeTest)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxWeaponRange)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getType)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getTooltip)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getWreckName)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getDeathExplosion)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getSelfDExplosion)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getTedClassString)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_getCategoryString)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanSelfD)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getSelfDCountdown)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanSubmerge)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanFly)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanMove)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanHover)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isFloater)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isBuilder)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isActivateWhenBuilt)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isOnOffable)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isFullHealthFactory)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isFactoryHeadingTakeoff)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isReclaimable)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCapturable)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanRestore)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanRepair)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanSelfRepair)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanReclaim)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanPatrol)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanFight)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanGuard)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanBuild)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanAssist)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanBeAssisted)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanRepeat)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanFireControl)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getFireState)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getMoveState)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getWingDrag)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getWingAngle)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getDrag)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFrontToSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getSpeedToFront)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMyGravity)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxBank)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxPitch)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTurnRadius)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getWantedHeight)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getVerticalSpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanCrash)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isHoverAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isAirStrafe)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getDlHoverFactor)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxAcceleration)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxDeceleration)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxAileron)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxElevator)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxRudder)(int teamId, int unitDefId);
///* returned size is 4 */
//const unsigned char*[] (CALLING_CONV *UnitDef_getYardMaps)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getXSize)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getYSize)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getBuildAngle)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getLoadingRadius)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getUnloadSpread)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getTransportCapacity)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getTransportSize)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getMinTransportSize)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isAirBase)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTransportMass)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMinTransportMass)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isHoldSteady)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isReleaseHeld)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCantBeTransported)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isTransportByEnemy)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getTransportUnloadMethod)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFallSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getUnitFallSpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanCloak)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isStartCloaked)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getCloakCost)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getCloakCostMoving)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getDecloakDistance)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isDecloakSpherical)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isDecloakOnFire)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanKamikaze)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getKamikazeDist)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isTargetingFacility)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanDGun)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isNeedGeo)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isFeature)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isHideDamage)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCommander)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isShowPlayerName)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanResurrect)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanCapture)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getHighTrajectoryType)(int teamId, int unitDefId);
unsigned int (CALLING_CONV *UnitDef_getNoChaseCategory)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isLeaveTracks)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTrackWidth)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTrackOffset)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTrackStrength)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getTrackStretch)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getTrackType)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanDropFlare)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFlareReloadTime)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFlareEfficiency)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getFlareDelay)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *UnitDef_getFlareDropVector)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getFlareTime)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getFlareSalvoSize)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getFlareSalvoDelay)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isSmoothAnim)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isMetalMaker)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isCanLoopbackAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isLevelGround)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isUseBuildingGroundDecal)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getBuildingDecalType)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getBuildingDecalSizeX)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getBuildingDecalSizeY)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getBuildingDecalDecaySpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isFirePlatform)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMaxFuel)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getRefuelTime)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_getMinAirBasePower)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getMaxThisUnit)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getDecoyDefId)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_isDontLand)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getShieldWeaponDefId)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getStockpileWeaponDefId)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getNumBuildOptions)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getBuildOptions)(int teamId, int unitDefId, int unitDefIds[]);
int (CALLING_CONV *UnitDef_getNumCustomParams)(int teamId, int unitDefId);
//int (CALLING_CONV *UnitDef_getCustomParams)(int teamId, int unitDefId, const char* map[][2]);
int (CALLING_CONV *UnitDef_getCustomParamKeys)(int teamId, int unitDefId, const char* keys[]);
int (CALLING_CONV *UnitDef_getCustomParamValues)(int teamId, int unitDefId, const char* values[]);
bool (CALLING_CONV *UnitDef_hasMoveData)(int teamId, int unitDefId);
/* enum MoveType { Ground_Move, Hover_Move, Ship_Move }; */
int (CALLING_CONV *UnitDef_MoveData_getMoveType)(int teamId, int unitDefId);
/* 0=tank,1=kbot,2=hover,3=ship */
int (CALLING_CONV *UnitDef_MoveData_getMoveFamily)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_MoveData_getSize)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getDepth)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getMaxSlope)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getSlopeMod)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getDepthMod)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_MoveData_getPathType)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getCrushStrength)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getMaxSpeed)(int teamId, int unitDefId);
short (CALLING_CONV *UnitDef_MoveData_getMaxTurnRate)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getMaxAcceleration)(int teamId, int unitDefId);
float (CALLING_CONV *UnitDef_MoveData_getMaxBreaking)(int teamId, int unitDefId);
bool (CALLING_CONV *UnitDef_MoveData_isSubMarine)(int teamId, int unitDefId);
int (CALLING_CONV *UnitDef_getNumUnitDefWeapons)(int teamId, int unitDefId);
const char* (CALLING_CONV *UnitDef_UnitDefWeapon_getName)(int teamId, int unitDefId, int weaponIndex);
int (CALLING_CONV *UnitDef_UnitDefWeapon_getWeaponDefId)(int teamId, int unitDefId, int weaponIndex);
int (CALLING_CONV *UnitDef_UnitDefWeapon_getSlavedTo)(int teamId, int unitDefId, int weaponIndex);
struct SAIFloat3 (CALLING_CONV *UnitDef_UnitDefWeapon_getMainDir)(int teamId, int unitDefId, int weaponIndex);
float (CALLING_CONV *UnitDef_UnitDefWeapon_getMaxAngleDif)(int teamId, int unitDefId, int weaponIndex);
float (CALLING_CONV *UnitDef_UnitDefWeapon_getFuelUsage)(int teamId, int unitDefId, int weaponIndex);
unsigned int (CALLING_CONV *UnitDef_UnitDefWeapon_getBadTargetCat)(int teamId, int unitDefId, int weaponIndex);
unsigned int (CALLING_CONV *UnitDef_UnitDefWeapon_getOnlyTargetCat)(int teamId, int unitDefId, int weaponIndex);
// END OBJECT UnitDef



// BEGINN OBJECT Unit
int (CALLING_CONV *Unit_STATIC_getLimit)(int teamId);
int (CALLING_CONV *Unit_STATIC_getEnemies)(int teamId, int unitIds[]);
int (CALLING_CONV *Unit_STATIC_getEnemiesIn)(int teamId, int unitIds[], struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Unit_STATIC_getEnemiesInRadarAndLos)(int teamId, int unitIds[]);
int (CALLING_CONV *Unit_STATIC_getFriendlies)(int teamId, int unitIds[]);
int (CALLING_CONV *Unit_STATIC_getFriendliesIn)(int teamId, int unitIds[], struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Unit_STATIC_getNeutrals)(int teamId, int unitIds[]);
int (CALLING_CONV *Unit_STATIC_getNeutralsIn)(int teamId, int unitIds[], struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Unit_STATIC_getSelected)(int teamId, int unitIds[]);
int (CALLING_CONV *Unit_STATIC_updateSelectedUnitsIcons)(int teamId);

int (CALLING_CONV *Unit_getDefId)(int teamId, int unitId);
int (CALLING_CONV *Unit_getAiHint)(int teamId, int unitId);
int (CALLING_CONV *Unit_getTeam)(int teamId, int unitId);
int (CALLING_CONV *Unit_getAllyTeam)(int teamId, int unitId);
int (CALLING_CONV *Unit_getStockpile)(int teamId, int unitId);
int (CALLING_CONV *Unit_getStockpileQueued)(int teamId, int unitId);
float (CALLING_CONV *Unit_getCurrentFuel)(int teamId, int unitId);
float (CALLING_CONV *Unit_getMaxSpeed)(int teamId, int unitId);
float (CALLING_CONV *Unit_getMaxRange)(int teamId, int unitId);
float (CALLING_CONV *Unit_getMaxHealth)(int teamId, int unitId);
float (CALLING_CONV *Unit_getExperience)(int teamId, int unitId);
int (CALLING_CONV *Unit_getGroup)(int teamId, int unitId);
int (CALLING_CONV *Unit_getNumCurrentCommands)(int teamId, int unitId);
/* for the type of the command queue, see CCommandQueue::CommandQueueType CommandQueue.h */
int (CALLING_CONV *Unit_CurrentCommands_getType)(int teamId, int unitId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Unit_CurrentCommands_getIds)(int teamId, int unitId, int ids[]);
int (CALLING_CONV *Unit_CurrentCommands_getOptions)(int teamId, int unitId, unsigned char options[]);
int (CALLING_CONV *Unit_CurrentCommands_getTag)(int teamId, int unitId, unsigned int tags[]);
int (CALLING_CONV *Unit_CurrentCommands_getTimeOut)(int teamId, int unitId, int timeOuts[]);
int (CALLING_CONV *Unit_CurrentCommands_getNumParams)(int teamId, int unitId, int numParams[]);
//int (CALLING_CONV *Unit_CurrentCommands_getParams)(int teamId, int unitId, float params[][2]);
int (CALLING_CONV *Unit_CurrentCommands_getParams)(int teamId, int unitId, unsigned int commandIndex, float params[]);
int (CALLING_CONV *Unit_getNumSupportedCommands)(int teamId, int unitId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Unit_SupportedCommands_getId)(int teamId, int unitId, int ids[]);
int (CALLING_CONV *Unit_SupportedCommands_getName)(int teamId, int unitId, const char* names[]);
int (CALLING_CONV *Unit_SupportedCommands_getToolTip)(int teamId, int unitId, const char* toolTips[]);
int (CALLING_CONV *Unit_SupportedCommands_isShowUnique)(int teamId, int unitId, bool showUniques[]);
int (CALLING_CONV *Unit_SupportedCommands_isDisabled)(int teamId, int unitId, bool disableds[]);
int (CALLING_CONV *Unit_SupportedCommands_getNumParams)(int teamId, int unitId, int numParams[]);
//int (CALLING_CONV *Unit_SupportedCommands_getParams)(int teamId, int unitId, const char* params[][2]);
int (CALLING_CONV *Unit_SupportedCommands_getParams)(int teamId, int unitId, unsigned int commandIndex, const char* params[]);
float (CALLING_CONV *Unit_getHealth)(int teamId, int unitId);
float (CALLING_CONV *Unit_getSpeed)(int teamId, int unitId);
float (CALLING_CONV *Unit_getPower)(int teamId, int unitId);
float (CALLING_CONV *Unit_ResourceInfo_Metal_getUse)(int teamId, int unitId);
float (CALLING_CONV *Unit_ResourceInfo_Metal_getMake)(int teamId, int unitId);
float (CALLING_CONV *Unit_ResourceInfo_Energy_getUse)(int teamId, int unitId);
float (CALLING_CONV *Unit_ResourceInfo_Energy_getMake)(int teamId, int unitId);
struct SAIFloat3 (CALLING_CONV *Unit_getPos)(int teamId, int unitId);
bool (CALLING_CONV *Unit_isActivated)(int teamId, int unitId);
bool (CALLING_CONV *Unit_isBeingBuilt)(int teamId, int unitId);
bool (CALLING_CONV *Unit_isCloaked)(int teamId, int unitId);
bool (CALLING_CONV *Unit_isParalyzed)(int teamId, int unitId);
bool (CALLING_CONV *Unit_isNeutral)(int teamId, int unitId);
int (CALLING_CONV *Unit_getBuildingFacing)(int teamId, int unitId);
int (CALLING_CONV *Unit_getLastUserOrderFrame)(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
int (CALLING_CONV *Group_getNumSupportedCommands)(int teamId, int groupId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Group_SupportedCommands_getId)(int teamId, int groupId, int ids[]);
int (CALLING_CONV *Group_SupportedCommands_getName)(int teamId, int groupId, const char* names[]);
int (CALLING_CONV *Group_SupportedCommands_getToolTip)(int teamId, int groupId, const char* toolTips[]);
int (CALLING_CONV *Group_SupportedCommands_isShowUnique)(int teamId, int groupId, bool showUniques[]);
int (CALLING_CONV *Group_SupportedCommands_isDisabled)(int teamId, int groupId, bool disableds[]);
int (CALLING_CONV *Group_SupportedCommands_getNumParams)(int teamId, int groupId, int numParams[]);
//int (CALLING_CONV *Group_SupportedCommands_getParams)(int teamId, int groupId, const char* params[][2]);
int (CALLING_CONV *Group_SupportedCommands_getParams)(int teamId, int groupId, unsigned int commandIndex, const char* params[]);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Group_OrderPreview_getId)(int teamId, int groupId);
unsigned char (CALLING_CONV *Group_OrderPreview_getOptions)(int teamId, int groupId);
unsigned int (CALLING_CONV *Group_OrderPreview_getTag)(int teamId, int groupId);
int (CALLING_CONV *Group_OrderPreview_getTimeOut)(int teamId, int groupId);
unsigned int (CALLING_CONV *Group_OrderPreview_getParams)(int teamId, int groupId, float params[], unsigned int maxParams);
bool (CALLING_CONV *Group_isSelected)(int teamId, int groupId);
// END OBJECT Group



// BEGINN OBJECT Mod
const char* (CALLING_CONV *Mod_getName)(int teamId);
// END OBJECT Mod



// BEGINN OBJECT Map
unsigned int (CALLING_CONV *Map_getChecksum)(int teamId);
struct SAIFloat3 (CALLING_CONV *Map_getStartPos)(int teamId);
struct SAIFloat3 (CALLING_CONV *Map_getMousePos)(int teamId);
bool (CALLING_CONV *Map_isPosInCamera)(int teamId, struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Map_getWidth)(int teamId);
int (CALLING_CONV *Map_getHeight)(int teamId);
const float* (CALLING_CONV *Map_getHeightMap)(int teamId);
float (CALLING_CONV *Map_getMinHeight)(int teamId);
float (CALLING_CONV *Map_getMaxHeight)(int teamId);
const float* (CALLING_CONV *Map_getSlopeMap)(int teamId);
const unsigned short* (CALLING_CONV *Map_getLosMap)(int teamId);
const unsigned short* (CALLING_CONV *Map_getRadarMap)(int teamId);
const unsigned short* (CALLING_CONV *Map_getJammerMap)(int teamId);
const unsigned char* (CALLING_CONV *Map_getMetalMap)(int teamId);
const char* (CALLING_CONV *Map_getName)(int teamId);
float (CALLING_CONV *Map_getElevationAt)(int teamId, float x, float z);
float (CALLING_CONV *Map_getMaxMetal)(int teamId);
float (CALLING_CONV *Map_getExtractorRadius)(int teamId);
float (CALLING_CONV *Map_getMinWind)(int teamId);
float (CALLING_CONV *Map_getMaxWind)(int teamId);
float (CALLING_CONV *Map_getTidalStrength)(int teamId);
float (CALLING_CONV *Map_getGravity)(int teamId);
//int (CALLING_CONV *Map_getPoints)(int teamId, struct SAIFloat3 positions[], unsigned char colors[][3], const char* labels[], int maxPoints);
//int (CALLING_CONV *Map_getLines)(int teamId, struct SAIFloat3 firstPositions[], struct SAIFloat3 secondPositions[], unsigned char colors[][3], int maxLines);
int (CALLING_CONV *Map_getPoints)(int teamId, struct SAIFloat3 positions[], struct SAIFloat3 colors[], const char* labels[], int maxPoints);
int (CALLING_CONV *Map_getLines)(int teamId, struct SAIFloat3 firstPositions[], struct SAIFloat3 secondPositions[], struct SAIFloat3 colors[], int maxLines);
bool (CALLING_CONV *Map_canBuildAt)(int teamId, int unitDefId, struct SAIFloat3 pos, int facing);
struct SAIFloat3 (CALLING_CONV *Map_findClosestBuildSite)(int teamId, int unitDefId, struct SAIFloat3 pos, float searchRadius, int minDist, int facing);
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
//const SAIFeatureDef* (CALLING_CONV *getFeatureDef)(int teamId, int featureDefId);
const char* (CALLING_CONV *FeatureDef_getName)(int teamId, int weaponDefId);
const char* (CALLING_CONV *FeatureDef_getDescription)(int teamId, int weaponDefId);
const char* (CALLING_CONV *FeatureDef_getFilename)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getId)(int teamId, int weaponDefId);
float (CALLING_CONV *FeatureDef_getMetal)(int teamId, int weaponDefId);
float (CALLING_CONV *FeatureDef_getEnergy)(int teamId, int weaponDefId);
float (CALLING_CONV *FeatureDef_getMaxHealth)(int teamId, int weaponDefId);
float (CALLING_CONV *FeatureDef_getReclaimTime)(int teamId, int weaponDefId);
float (CALLING_CONV *FeatureDef_getMass)(int teamId, int weaponDefId);
const char* (CALLING_CONV *FeatureDef_getCollisionVolumeType)(int teamId, int weaponDefId);
struct SAIFloat3 (CALLING_CONV *FeatureDef_getCollisionVolumeScales)(int teamId, int weaponDefId);
struct SAIFloat3 (CALLING_CONV *FeatureDef_getCollisionVolumeOffsets)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getCollisionVolumeTest)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isUpright)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getDrawType)(int teamId, int weaponDefId);
const char* (CALLING_CONV *FeatureDef_getModelName)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getModelType)(int teamId, int weaponDefId);
/** -1 := only if it is the 1st wreckage of the unitdef (default), 0 := no it isn't, 1 := yes it is */
int (CALLING_CONV *FeatureDef_getResurrectable)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getSmokeTime)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isDestructable)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isReclaimable)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isBlocking)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isBurnable)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isFloating)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isNoSelect)(int teamId, int weaponDefId);
bool (CALLING_CONV *FeatureDef_isGeoThermal)(int teamId, int weaponDefId);
const char* (CALLING_CONV *FeatureDef_getDeathFeature)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getXsize)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getYsize)(int teamId, int weaponDefId);
int (CALLING_CONV *FeatureDef_getNumCustomParams)(int teamId, int weaponDefId);
//int (CALLING_CONV *FeatureDef_getCustomParams)(int teamId, int weaponDefId, const char* map[][2]);
int (CALLING_CONV *FeatureDef_getCustomParamKeys)(int teamId, int weaponDefId, const char* keys[]);
int (CALLING_CONV *FeatureDef_getCustomParamValues)(int teamId, int weaponDefId, const char* values[]);
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
int (CALLING_CONV *Feature_STATIC_getIds)(int teamId, int *featureIds, int max);
int (CALLING_CONV *Feature_STATIC_getIdsIn)(int teamId, int *featureIds, int max, struct SAIFloat3 pos, float radius);

int (CALLING_CONV *Feature_getDefId)(int teamId, int featureId);
float (CALLING_CONV *Feature_getHealth)(int teamId, int featureId);
float (CALLING_CONV *Feature_getReclaimLeft)(int teamId, int featureId);
struct SAIFloat3 (CALLING_CONV *Feature_getPos)(int teamId, int featureId);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
int (CALLING_CONV *WeaponDef_STATIC_getIdByName)(int teamId, const char* weaponDefName);
int (CALLING_CONV *WeaponDef_STATIC_getNumDamageTypes)(int teamId);

//const SAIWeaponDef* (CALLING_CONV *getWeaponDef)(int teamId, int weaponDefId);
const char* (CALLING_CONV *WeaponDef_getName)(int teamId, int weaponDefId);
const char* (CALLING_CONV *WeaponDef_getType)(int teamId, int weaponDefId);
const char* (CALLING_CONV *WeaponDef_getDescription)(int teamId, int weaponDefId);
const char* (CALLING_CONV *WeaponDef_getFilename)(int teamId, int weaponDefId);
const char* (CALLING_CONV *WeaponDef_getCegTag)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getRange)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getHeightMod)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getAccuracy)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getSprayAngle)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getMovingAccuracy)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getTargetMoveError)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getLeadLimit)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getLeadBonus)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getPredictBoost)(int teamId, int weaponDefId);

//DamageArray (CALLING_CONV *WeaponDef_getDamages)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_Damages_getParalyzeDamageTime)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_Damages_getImpulseFactor)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_Damages_getImpulseBoost)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_Damages_getCraterMult)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_Damages_getCraterBoost)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_Damages_getNumTypes)(int teamId, int weaponDefId);
//float (CALLING_CONV *WeaponDef_Damages_getType)(int teamId, int weaponDefId, int typeIndex);
void (CALLING_CONV *WeaponDef_Damages_getTypeDamages)(int teamId, int weaponDefId, float typeDamages[]);

float (CALLING_CONV *WeaponDef_getAreaOfEffect)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isNoSelfDamage)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getFireStarter)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getEdgeEffectiveness)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getSize)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getSizeGrowth)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getCollisionSize)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getSalvoSize)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getSalvoDelay)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getReload)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getBeamTime)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isBeamBurst)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isWaterBounce)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isGroundBounce)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getBounceRebound)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getBounceSlip)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getNumBounce)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getMaxAngle)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getRestTime)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getUpTime)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getFlightTime)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getMetalCost)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getEnergyCost)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getSupplyCost)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getProjectilesPerShot)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getId)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getTdfId)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isTurret)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isOnlyForward)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isFixedLauncher)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isWaterWeapon)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isFireSubmersed)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isSubMissile)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isTracks)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isDropped)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isParalyzer)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isImpactOnly)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isNoAutoTarget)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isManualFire)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getInterceptor)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getTargetable)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isStockpileable)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getCoverageRange)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getStockpileTime)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getIntensity)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getThickness)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getLaserFlareSize)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getCoreThickness)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getDuration)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getLodDistance)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getFalloffRate)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getGraphicsType)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isSoundTrigger)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isSelfExplode)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isGravityAffected)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getHighTrajectory)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getMyGravity)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isNoExplode)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getStartVelocity)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getWeaponAcceleration)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getTurnRate)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getMaxVelocity)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getProjectileSpeed)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getExplosionSpeed)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *WeaponDef_getOnlyTargetCategory)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getWobble)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getDance)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getTrajectoryHeight)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isLargeBeamLaser)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isShieldRepulser)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isSmartShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isExteriorShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isVisibleShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isVisibleShieldRepulse)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getVisibleShieldHitFrames)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldEnergyUse)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldRadius)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldForce)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldMaxSpeed)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldPower)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldPowerRegen)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldPowerRegenEnergy)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldStartingPower)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getShieldRechargeDelay)(int teamId, int weaponDefId);
struct SAIFloat3 (CALLING_CONV *WeaponDef_getShieldGoodColor)(int teamId, int weaponDefId);
struct SAIFloat3 (CALLING_CONV *WeaponDef_getShieldBadColor)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getShieldAlpha)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *WeaponDef_getShieldInterceptType)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *WeaponDef_getInterceptedByShieldType)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isAvoidFriendly)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isAvoidFeature)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isAvoidNeutral)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getTargetBorder)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getCylinderTargetting)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getMinIntensity)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getHeightBoostFactor)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getProximityPriority)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *WeaponDef_getCollisionFlags)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isSweepFire)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isCanAttackGround)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getCameraShake)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getDynDamageExp)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getDynDamageMin)(int teamId, int weaponDefId);
float (CALLING_CONV *WeaponDef_getDynDamageRange)(int teamId, int weaponDefId);
bool (CALLING_CONV *WeaponDef_isDynDamageInverted)(int teamId, int weaponDefId);
int (CALLING_CONV *WeaponDef_getNumCustomParams)(int teamId, int weaponDefId);
//const char*[] (CALLING_CONV *WeaponDef_getCustomParam)(int teamId, int weaponDefId, int index);
//int (CALLING_CONV *WeaponDef_getCustomParams)(int teamId, int weaponDefId, const char* map[][2]);
int (CALLING_CONV *WeaponDef_getCustomParamKeys)(int teamId, int weaponDefId, const char* keys[]);
int (CALLING_CONV *WeaponDef_getCustomParamValues)(int teamId, int weaponDefId, const char* values[]);
// END OBJECT WeaponDef

};

#ifdef	__cplusplus
}	// extern "C"
#endif

#if defined __cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
class IGlobalAICallback;
SAICallback* initSAICallback(int teamId, IGlobalAICallback* aiGlobalCallback);
#endif	// defined __cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE

#endif	// _SAICALLBACK_H

