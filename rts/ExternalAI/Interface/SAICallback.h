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
int (CALLING_CONV *Clb_handleCommand)(int teamId, int toId, int commandId, int commandTopic, void* commandData);


// BEGINN misc callback functions
int (CALLING_CONV *Clb_Game_getCurrentFrame)(int teamId); //TODO: deprecate, becuase we get the frame from the SUpdateEvent
int (CALLING_CONV *Clb_Game_getAiInterfaceVersion)(int teamId);
int (CALLING_CONV *Clb_Game_getMyTeam)(int teamId);
int (CALLING_CONV *Clb_Game_getMyAllyTeam)(int teamId);
int (CALLING_CONV *Clb_Game_getPlayerTeam)(int teamId, int playerId);
const char* (CALLING_CONV *Clb_Game_getTeamSide)(int teamId, int otherTeamId);
bool (CALLING_CONV *Clb_Game_isExceptionHandlingEnabled)(int teamId);
bool (CALLING_CONV *Clb_Game_isDebugModeEnabled)(int teamId);
int (CALLING_CONV *Clb_Game_getMode)(int teamId);
bool (CALLING_CONV *Clb_Game_isPaused)(int teamId);
float (CALLING_CONV *Clb_Game_getSpeedFactor)(int teamId);
const char* (CALLING_CONV *Clb_Game_getSetupScript)(int teamId);
// END misc callback functions


// BEGINN Visualization related callback functions
float (CALLING_CONV *Clb_Gui_getViewRange)(int teamId);
float (CALLING_CONV *Clb_Gui_getScreenX)(int teamId);
float (CALLING_CONV *Clb_Gui_getScreenY)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Gui_Camera_getDirection)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Gui_Camera_getPosition)(int teamId);
// END Visualization related callback functions


// BEGINN kind of deprecated; it is recommended not to use these
//bool (CALLING_CONV *Clb_getProperty)(int teamId, int id, int property, void* dst);
//bool (CALLING_CONV *Clb_getValue)(int teamId, int id, void* dst);
// END kind of deprecated; it is recommended not to use these


// BEGINN OBJECT Cheats
bool (CALLING_CONV *Clb_Cheats_isEnabled)(int teamId);
bool (CALLING_CONV *Clb_Cheats_setEnabled)(int teamId, bool enable);
bool (CALLING_CONV *Clb_Cheats_setEventsEnabled)(int teamId, bool enabled);
bool (CALLING_CONV *Clb_Cheats_isOnlyPassive)(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT Resource
int (CALLING_CONV *Clb_0MULTI1SIZE0Resource)(int teamId);
int (CALLING_CONV *Clb_0MULTI1FETCH3ResourceByName0Resource)(int teamId, const char* resourceName);
const char* (CALLING_CONV *Clb_Resource_getName)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Resource_getOptimum)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getCurrent)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getIncome)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getUsage)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Economy_0REF1Resource2resourceId0getStorage)(int teamId, int resourceId);
// END OBJECT Resource


// BEGINN OBJECT File
int (CALLING_CONV *Clb_File_getSize)(int teamId, const char* fileName);
bool (CALLING_CONV *Clb_File_getContent)(int teamId, const char* fileName, void* buffer, int bufferLen);
bool (CALLING_CONV *Clb_File_locateForReading)(int teamId, char* fileName);
bool (CALLING_CONV *Clb_File_locateForWriting)(int teamId, char* fileName);
// END OBJECT File


// BEGINN OBJECT UnitDef
int (CALLING_CONV *Clb_0MULTI1SIZE0UnitDef)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0UnitDef)(int teamId, int unitDefIds[], int unitDefIds_max);
int (CALLING_CONV *Clb_0MULTI1FETCH3UnitDefByName0UnitDef)(int teamId, const char* unitName);
//int (CALLING_CONV *Clb_UnitDef_getId)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getHeight)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getRadius)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isValid)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getName)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getHumanName)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getFileName)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getAiHint)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getCobId)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTechLevel)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getGaia)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getUpkeep)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getResourceMake)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getMakesResource)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getCost)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0getStorage)(int teamId, int unitDefId, int resourceId);
float (CALLING_CONV *Clb_UnitDef_getBuildTime)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getAutoHeal)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getIdleAutoHeal)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getIdleTime)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getPower)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getHealth)(int teamId, int unitDefId);
unsigned int (CALLING_CONV *Clb_UnitDef_getCategory)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTurnRate)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isTurnInPlace)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMoveType)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isUpright)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isCollide)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getControlRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getLosRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getAirLosRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getLosHeight)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getRadarRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSonarRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getJammerRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSonarJamRadius)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSeismicRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSeismicSignature)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isStealth)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isSonarStealth)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isBuildRange3D)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getBuildDistance)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getBuildSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getReclaimSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getRepairSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxRepairSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getResurrectSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getCaptureSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTerraformSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMass)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isPushResistant)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isStrafeToAttack)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinCollisionSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSlideTolerance)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxSlope)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxHeightDif)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinWaterDepth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWaterline)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxWaterDepth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getArmoredMultiple)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getArmorType)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMode)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_FlankingBonus_getDir)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMax)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMin)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_FlankingBonus_getMobilityAdd)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_CollisionVolume_getType)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_CollisionVolume_getScales)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_CollisionVolume_getOffsets)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_CollisionVolume_getTest)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxWeaponRange)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getType)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getTooltip)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getWreckName)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getDeathExplosion)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getSelfDExplosion)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getTedClassString)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_getCategoryString)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToSelfD)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getSelfDCountdown)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToSubmerge)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToFly)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToMove)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToHover)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFloater)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isBuilder)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isActivateWhenBuilt)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isOnOffable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFullHealthFactory)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFactoryHeadingTakeoff)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isReclaimable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isCapturable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToRestore)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToRepair)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToSelfRepair)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToReclaim)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToPatrol)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToFight)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToGuard)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToBuild)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToAssist)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAssistable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToRepeat)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToFireControl)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFireState)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMoveState)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWingDrag)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWingAngle)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getDrag)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFrontToSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getSpeedToFront)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMyGravity)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxBank)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxPitch)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTurnRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getWantedHeight)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getVerticalSpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToCrash)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isHoverAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAirStrafe)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getDlHoverFactor)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxAcceleration)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxDeceleration)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxAileron)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxElevator)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxRudder)(int teamId, int unitDefId);
///* returned size is 4 */
//const unsigned char*[] (CALLING_CONV *Clb_UnitDef_getYardMaps)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getXSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getZSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildAngle)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getLoadingRadius)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getUnloadSpread)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTransportCapacity)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTransportSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMinTransportSize)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAirBase)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTransportMass)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinTransportMass)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isHoldSteady)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isReleaseHeld)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isNotTransportable)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isTransportByEnemy)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTransportUnloadMethod)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFallSpeed)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getUnitFallSpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToCloak)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isStartCloaked)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getCloakCost)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getCloakCostMoving)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getDecloakDistance)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isDecloakSpherical)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isDecloakOnFire)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToKamikaze)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getKamikazeDist)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isTargetingFacility)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToDGun)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isNeedGeo)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFeature)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isHideDamage)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isCommander)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isShowPlayerName)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToResurrect)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToCapture)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getHighTrajectoryType)(int teamId, int unitDefId);
unsigned int (CALLING_CONV *Clb_UnitDef_getNoChaseCategory)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isLeaveTracks)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackWidth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackOffset)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackStrength)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getTrackStretch)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getTrackType)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToDropFlare)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFlareReloadTime)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFlareEfficiency)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getFlareDelay)(int teamId, int unitDefId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_getFlareDropVector)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFlareTime)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFlareSalvoSize)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getFlareSalvoDelay)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isSmoothAnim)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker)(int teamId, int unitDefId, int resourceId);
bool (CALLING_CONV *Clb_UnitDef_isAbleToLoopbackAttack)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isLevelGround)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isUseBuildingGroundDecal)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildingDecalType)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildingDecalSizeX)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getBuildingDecalSizeY)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getBuildingDecalDecaySpeed)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isFirePlatform)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMaxFuel)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getRefuelTime)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_getMinAirBasePower)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_getMaxThisUnit)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_isDontLand)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef)(int teamId, int unitDefId);
//Clb_Unit_0MULTI1SIZE1Command0CurrentCommand
int (CALLING_CONV *Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions)(int teamId, int unitDefId, int unitDefIds[], int unitDefIds_max);
//int (CALLING_CONV *Clb_UnitDef_0MULTI1SIZE1UnitDef0BuildOptions)(int teamId, int unitDefId);
//int (CALLING_CONV *Clb_UnitDef_0MULTI1VALS1UnitDef0BuildOptions)(int teamId, int unitDefId, int unitDefIds[], int unitDefIds_max);
//int (CALLING_CONV *Clb_UnitDef_0MULTI1SIZE1BuildOption0UnitDef)(int teamId, int unitDefId);
//int (CALLING_CONV *Clb_UnitDef_0MULTI1VALS1BuildOption0UnitDef)(int teamId, int unitDefId, int unitDefIds[], int unitDefIds_max);
int (CALLING_CONV *Clb_UnitDef_0MAP1SIZE0getCustomParams)(int teamId, int unitDefId);
void (CALLING_CONV *Clb_UnitDef_0MAP1KEYS0getCustomParams)(int teamId, int unitDefId, const char* keys[]);
void (CALLING_CONV *Clb_UnitDef_0MAP1VALS0getCustomParams)(int teamId, int unitDefId, const char* values[]);
bool (CALLING_CONV *Clb_UnitDef_0AVAILABLE0MoveData)(int teamId, int unitDefId);
/* enum MoveType { Ground_Move, Hover_Move, Ship_Move }; */
int (CALLING_CONV *Clb_UnitDef_MoveData_getMoveType)(int teamId, int unitDefId);
/* 0=tank,1=kbot,2=hover,3=ship */
int (CALLING_CONV *Clb_UnitDef_MoveData_getMoveFamily)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_MoveData_getSize)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getDepth)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxSlope)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getSlopeMod)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getDepthMod)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_MoveData_getPathType)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getCrushStrength)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxSpeed)(int teamId, int unitDefId);
short (CALLING_CONV *Clb_UnitDef_MoveData_getMaxTurnRate)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxAcceleration)(int teamId, int unitDefId);
float (CALLING_CONV *Clb_UnitDef_MoveData_getMaxBreaking)(int teamId, int unitDefId);
bool (CALLING_CONV *Clb_UnitDef_MoveData_isSubMarine)(int teamId, int unitDefId);
int (CALLING_CONV *Clb_UnitDef_0MULTI1SIZE0WeaponMount)(int teamId, int unitDefId);
const char* (CALLING_CONV *Clb_UnitDef_WeaponMount_getName)(int teamId, int unitDefId, int weaponMountId);
int (CALLING_CONV *Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef)(int teamId, int unitDefId, int weaponMountId);
int (CALLING_CONV *Clb_UnitDef_WeaponMount_getSlavedTo)(int teamId, int unitDefId, int weaponMountId);
struct SAIFloat3 (CALLING_CONV *Clb_UnitDef_WeaponMount_getMainDir)(int teamId, int unitDefId, int weaponMountId);
float (CALLING_CONV *Clb_UnitDef_WeaponMount_getMaxAngleDif)(int teamId, int unitDefId, int weaponMountId);
float (CALLING_CONV *Clb_UnitDef_WeaponMount_getFuelUsage)(int teamId, int unitDefId, int weaponMountId);
unsigned int (CALLING_CONV *Clb_UnitDef_WeaponMount_getBadTargetCategory)(int teamId, int unitDefId, int weaponMountId);
unsigned int (CALLING_CONV *Clb_UnitDef_WeaponMount_getOnlyTargetCategory)(int teamId, int unitDefId, int weaponMountId);
// END OBJECT UnitDef



// BEGINN OBJECT Unit
/**
 * Returns the numnber of units a team can have, after which it can not build
 * any more. It is possible that a team has more units then this value at some
 * point in the game. This is possible for example with taking, reclaiming or
 * capturing units.
 * This value is usefull for controlling game performance (eg. setting it could
 * possibly will prevent old PCs from lagging becuase of games with lots of
 * units).
 */
int (CALLING_CONV *Clb_Unit_0STATIC0getLimit)(int teamId);
/**
 * Returns all units that are not in this teams ally-team nor neutral and are
 * in LOS.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3EnemyUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3EnemyUnits0Unit)(int teamId, int unitIds[], int unitIds_max);
/**
 * Returns all units that are not in this teams ally-team nor neutral and are
 * in LOS plus they have to be located in the specified area of the map.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3EnemyUnitsIn0Unit)(int teamId, struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3EnemyUnitsIn0Unit)(int teamId, struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
/**
 * Returns all units that are not in this teams ally-team nor neutral and are in
 * some way visible (sight or radar).
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit)(int teamId, int unitIds[], int unitIds_max);
/**
 * Returns all units that are in this teams ally-team, including this teams
 * units.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3FriendlyUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3FriendlyUnits0Unit)(int teamId, int unitIds[], int unitIds_max);
/**
 * Returns all units that are in this teams ally-team, including this teams
 * units plus they have to be located in the specified area of the map.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3FriendlyUnitsIn0Unit)(int teamId, struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3FriendlyUnitsIn0Unit)(int teamId, struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
/**
 * Returns all units that are neutral and are in LOS.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3NeutralUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3NeutralUnits0Unit)(int teamId, int unitIds[], int unitIds_max);
/**
 * Returns all units that are neutral and are in LOS plus they have to be
 * located in the specified area of the map.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3NeutralUnitsIn0Unit)(int teamId, struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3NeutralUnitsIn0Unit)(int teamId, struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
/**
 * Returns all units that are of the team controlled by this AI instance. This
 * list can also be created dynamically by the AI, through updating a list on
 * each unit-created and unit-destroyed event.
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3TeamUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3TeamUnits0Unit)(int teamId, int unitIds[], int unitIds_max);
/**
 * Returns all units that are currently selected (usually only contains units if
 * a human payer is controlling this team as well).
 */
int (CALLING_CONV *Clb_0MULTI1SIZE3SelectedUnits0Unit)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS3SelectedUnits0Unit)(int teamId, int unitIds[], int unitIds_max);
//void (CALLING_CONV *Clb_Unit_0STATIC0updateSelectedUnitsIcons)(int teamId);

int (CALLING_CONV *Clb_Unit_0SINGLE1FETCH2UnitDef0getDef)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getAiHint)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getTeam)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getAllyTeam)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getStockpile)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getStockpileQueued)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getCurrentFuel)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getMaxSpeed)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getMaxRange)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getMaxHealth)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getExperience)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getGroup)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE1Command0CurrentCommand)(int teamId, int unitId);
/* for the type of the command queue, see CCommandQueue::CommandQueueType CommandQueue.h */
int (CALLING_CONV *Clb_Unit_CurrentCommand_0STATIC0getType)(int teamId, int unitId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Clb_Unit_CurrentCommand_getId)(int teamId, int unitId, int commandId);
unsigned char (CALLING_CONV *Clb_Unit_CurrentCommand_getOptions)(int teamId, int unitId, int commandId);
unsigned int (CALLING_CONV *Clb_Unit_CurrentCommand_getTag)(int teamId, int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_CurrentCommand_getTimeOut)(int teamId, int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams)(int teamId, int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams)(int teamId, int unitId, int commandId, float params[], int params_max);
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE0SupportedCommand)(int teamId, int unitId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Clb_Unit_SupportedCommand_getId)(int teamId, int unitId, int commandId);
const char* (CALLING_CONV *Clb_Unit_SupportedCommand_getName)(int teamId, int unitId, int commandId);
const char* (CALLING_CONV *Clb_Unit_SupportedCommand_getToolTip)(int teamId, int unitId, int commandId);
bool (CALLING_CONV *Clb_Unit_SupportedCommand_isShowUnique)(int teamId, int unitId, int commandId);
bool (CALLING_CONV *Clb_Unit_SupportedCommand_isDisabled)(int teamId, int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams)(int teamId, int unitId, int commandId);
int (CALLING_CONV *Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams)(int teamId, int unitId, int commandId, const char* params[], int params_max);
float (CALLING_CONV *Clb_Unit_getHealth)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getSpeed)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_getPower)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_0MULTI1SIZE0ResourceInfo)(int teamId, int unitId);
float (CALLING_CONV *Clb_Unit_0REF1Resource2resourceId0getResourceUse)(int teamId, int unitId, int resourceId);
float (CALLING_CONV *Clb_Unit_0REF1Resource2resourceId0getResourceMake)(int teamId, int unitId, int resourceId);
struct SAIFloat3 (CALLING_CONV *Clb_Unit_getPos)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isActivated)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isBeingBuilt)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isCloaked)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isParalyzed)(int teamId, int unitId);
bool (CALLING_CONV *Clb_Unit_isNeutral)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getBuildingFacing)(int teamId, int unitId);
int (CALLING_CONV *Clb_Unit_getLastUserOrderFrame)(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
int (CALLING_CONV *Clb_0MULTI1SIZE0Group)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0Group)(int teamId, int groupIds[], int groupIds_max);
int (CALLING_CONV *Clb_Group_0MULTI1SIZE0SupportedCommand)(int teamId, int groupId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Clb_Group_SupportedCommand_getId)(int teamId, int groupId, int commandId);
const char* (CALLING_CONV *Clb_Group_SupportedCommand_getName)(int teamId, int groupId, int commandId);
const char* (CALLING_CONV *Clb_Group_SupportedCommand_getToolTip)(int teamId, int groupId, int commandId);
bool (CALLING_CONV *Clb_Group_SupportedCommand_isShowUnique)(int teamId, int groupId, int commandId);
bool (CALLING_CONV *Clb_Group_SupportedCommand_isDisabled)(int teamId, int groupId, int commandId);
int (CALLING_CONV *Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams)(int teamId, int groupId, int commandId);
int (CALLING_CONV *Clb_Group_SupportedCommand_0ARRAY1VALS0getParams)(int teamId, int groupId, int commandId, const char* params[], int params_max);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (CALLING_CONV *Clb_Group_OrderPreview_getId)(int teamId, int groupId);
unsigned char (CALLING_CONV *Clb_Group_OrderPreview_getOptions)(int teamId, int groupId);
unsigned int (CALLING_CONV *Clb_Group_OrderPreview_getTag)(int teamId, int groupId);
int (CALLING_CONV *Clb_Group_OrderPreview_getTimeOut)(int teamId, int groupId);
int (CALLING_CONV *Clb_Group_OrderPreview_0ARRAY1SIZE0getParams)(int teamId, int groupId);
int (CALLING_CONV *Clb_Group_OrderPreview_0ARRAY1VALS0getParams)(int teamId, int groupId, float params[], int params_max);
bool (CALLING_CONV *Clb_Group_isSelected)(int teamId, int groupId);
// END OBJECT Group



// BEGINN OBJECT Mod
const char* (CALLING_CONV *Clb_Mod_getName)(int teamId);
// END OBJECT Mod



// BEGINN OBJECT Map
unsigned int (CALLING_CONV *Clb_Map_getChecksum)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_getStartPos)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_getMousePos)(int teamId);
bool (CALLING_CONV *Clb_Map_isPosInCamera)(int teamId, struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_Map_getWidth)(int teamId);
int (CALLING_CONV *Clb_Map_getHeight)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getHeightMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getHeightMap)(int teamId, float heights[], int heights_max);
float (CALLING_CONV *Clb_Map_getMinHeight)(int teamId);
float (CALLING_CONV *Clb_Map_getMaxHeight)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getSlopeMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getSlopeMap)(int teamId, float slopes[], int slopes_max);
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getLosMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getLosMap)(int teamId, unsigned short losValues[], int losValues_max);
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getRadarMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getRadarMap)(int teamId, unsigned short radarValues[], int radarValues_max);
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0getJammerMap)(int teamId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0getJammerMap)(int teamId, unsigned short jammerValues[], int jammerValues_max);
int (CALLING_CONV *Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMap)(int teamId, int resourceId);
int (CALLING_CONV *Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMap)(int teamId, int resourceId, unsigned char resources[], int resources_max);
const char* (CALLING_CONV *Clb_Map_getName)(int teamId);
float (CALLING_CONV *Clb_Map_getElevationAt)(int teamId, float x, float z);
float (CALLING_CONV *Clb_Map_0REF1Resource2resourceId0getMaxResource)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Map_0REF1Resource2resourceId0getExtractorRadius)(int teamId, int resourceId);
float (CALLING_CONV *Clb_Map_getMinWind)(int teamId);
float (CALLING_CONV *Clb_Map_getMaxWind)(int teamId);
float (CALLING_CONV *Clb_Map_getTidalStrength)(int teamId);
float (CALLING_CONV *Clb_Map_getGravity)(int teamId);
//int (CALLING_CONV *Clb_Map_getPoints)(int teamId, struct SAIFloat3 positions[], unsigned char colors[][3], const char* labels[], int maxPoints);
//int (CALLING_CONV *Clb_Map_getLines)(int teamId, struct SAIFloat3 firstPositions[], struct SAIFloat3 secondPositions[], unsigned char colors[][3], int maxLines);
//int (CALLING_CONV *Clb_Map_getPoints)(int teamId, struct SAIFloat3 positions[], struct SAIFloat3 colors[], const char* labels[], int maxPoints);
int (CALLING_CONV *Clb_Map_0MULTI1SIZE0Point)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Point_getPosition)(int teamId, int pointId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Point_getColor)(int teamId, int pointId);
const char* (CALLING_CONV *Clb_Map_Point_getLabel)(int teamId, int pointId);
//int (CALLING_CONV *Clb_Map_getLines)(int teamId, struct SAIFloat3 firstPositions[], struct SAIFloat3 secondPositions[], struct SAIFloat3 colors[], int maxLines);
int (CALLING_CONV *Clb_Map_0MULTI1SIZE0Line)(int teamId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Line_getFirstPosition)(int teamId, int lineId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Line_getSecondPosition)(int teamId, int lineId);
struct SAIFloat3 (CALLING_CONV *Clb_Map_Line_getColor)(int teamId, int lineId);
bool (CALLING_CONV *Clb_Map_isPossibleToBuildAt)(int teamId, int unitDefId, struct SAIFloat3 pos, int facing);
struct SAIFloat3 (CALLING_CONV *Clb_Map_findClosestBuildSite)(int teamId, int unitDefId, struct SAIFloat3 pos, float searchRadius, int minDist, int facing);
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
int (CALLING_CONV *Clb_0MULTI1SIZE0FeatureDef)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0FeatureDef)(int teamId, int featureDefIds[], int featureDefIds_max);
int (CALLING_CONV *Clb_FeatureDef_getId)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getName)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getDescription)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getFileName)(int teamId, int featureDefId);
float (CALLING_CONV *Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource)(int teamId, int featureDefId, int resourceId);
float (CALLING_CONV *Clb_FeatureDef_getMaxHealth)(int teamId, int featureDefId);
float (CALLING_CONV *Clb_FeatureDef_getReclaimTime)(int teamId, int featureDefId);
float (CALLING_CONV *Clb_FeatureDef_getMass)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getType)(int teamId, int featureDefId);
struct SAIFloat3 (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getScales)(int teamId, int featureDefId);
struct SAIFloat3 (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getOffsets)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_CollisionVolume_getTest)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isUpright)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getDrawType)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getModelName)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getModelType)(int teamId, int featureDefId);
/** -1 := only if it is the 1st wreckage of the unitdef (default), 0 := no it isn't, 1 := yes it is */
int (CALLING_CONV *Clb_FeatureDef_getResurrectable)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getSmokeTime)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isDestructable)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isReclaimable)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isBlocking)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isBurnable)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isFloating)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isNoSelect)(int teamId, int featureDefId);
bool (CALLING_CONV *Clb_FeatureDef_isGeoThermal)(int teamId, int featureDefId);
const char* (CALLING_CONV *Clb_FeatureDef_getDeathFeature)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getXSize)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_getZSize)(int teamId, int featureDefId);
int (CALLING_CONV *Clb_FeatureDef_0MAP1SIZE0getCustomParams)(int teamId, int featureDefId);
void (CALLING_CONV *Clb_FeatureDef_0MAP1KEYS0getCustomParams)(int teamId, int featureDefId, const char* keys[]);
void (CALLING_CONV *Clb_FeatureDef_0MAP1VALS0getCustomParams)(int teamId, int featureDefId, const char* values[]);
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
int (CALLING_CONV *Clb_0MULTI1SIZE0Feature)(int teamId);
int (CALLING_CONV *Clb_0MULTI1VALS0Feature)(int teamId, int featureIds[], int featureIds_max);
int (CALLING_CONV *Clb_0MULTI1SIZE3FeaturesIn0Feature)(int teamId, struct SAIFloat3 pos, float radius);
int (CALLING_CONV *Clb_0MULTI1VALS3FeaturesIn0Feature)(int teamId, struct SAIFloat3 pos, float radius, int featureIds[], int featureIds_max);
int (CALLING_CONV *Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef)(int teamId, int featureId);
float (CALLING_CONV *Clb_Feature_getHealth)(int teamId, int featureId);
float (CALLING_CONV *Clb_Feature_getReclaimLeft)(int teamId, int featureId);
struct SAIFloat3 (CALLING_CONV *Clb_Feature_getPosition)(int teamId, int featureId);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
int (CALLING_CONV *Clb_0MULTI1SIZE0WeaponDef)(int teamId);
//int (CALLING_CONV *Clb_0MULTI1VALS0WeaponDef)(int teamId, int weaponDefIds[], int weaponDefIds_max);
int (CALLING_CONV *Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef)(int teamId, const char* weaponDefName);
//const SAIWeaponDef* (CALLING_CONV *Clb_getWeaponDef)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getName)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getType)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getDescription)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getFileName)(int teamId, int weaponDefId);
const char* (CALLING_CONV *Clb_WeaponDef_getCegTag)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getRange)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getHeightMod)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getAccuracy)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSprayAngle)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMovingAccuracy)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getTargetMoveError)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getLeadLimit)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getLeadBonus)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getPredictBoost)(int teamId, int weaponDefId);

// Deprecate the following function, if no longer needed by legacy Cpp AIs
int (CALLING_CONV *Clb_WeaponDef_0STATIC0getNumDamageTypes)(int teamId);
//DamageArray (CALLING_CONV *Clb_WeaponDef_getDamages)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_Damage_getParalyzeDamageTime)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getImpulseFactor)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getImpulseBoost)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getCraterMult)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Damage_getCraterBoost)(int teamId, int weaponDefId);
//float (CALLING_CONV *Clb_WeaponDef_Damage_getType)(int teamId, int weaponDefId, int typeId);
int (CALLING_CONV *Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes)(int teamId, int weaponDefId, float types[], int types_max);

int (CALLING_CONV *Clb_WeaponDef_getId)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getAreaOfEffect)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isNoSelfDamage)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getFireStarter)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getEdgeEffectiveness)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSize)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSizeGrowth)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCollisionSize)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getSalvoSize)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getSalvoDelay)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getReload)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getBeamTime)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isBeamBurst)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isWaterBounce)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isGroundBounce)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getBounceRebound)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getBounceSlip)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getNumBounce)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMaxAngle)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getRestTime)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getUpTime)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getFlightTime)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_0REF1Resource2resourceId0getCost)(int teamId, int weaponDefId, int resourceId);
float (CALLING_CONV *Clb_WeaponDef_getSupplyCost)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getProjectilesPerShot)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getTdfId)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isTurret)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isOnlyForward)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isFixedLauncher)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isWaterWeapon)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isFireSubmersed)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSubMissile)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isTracks)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isDropped)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isParalyzer)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isImpactOnly)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isNoAutoTarget)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isManualFire)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getInterceptor)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getTargetable)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isStockpileable)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCoverageRange)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getStockpileTime)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getIntensity)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getThickness)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getLaserFlareSize)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCoreThickness)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDuration)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getLodDistance)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getFalloffRate)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getGraphicsType)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSoundTrigger)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSelfExplode)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isGravityAffected)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getHighTrajectory)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMyGravity)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isNoExplode)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getStartVelocity)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getWeaponAcceleration)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getTurnRate)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMaxVelocity)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getProjectileSpeed)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getExplosionSpeed)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *Clb_WeaponDef_getOnlyTargetCategory)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getWobble)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDance)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getTrajectoryHeight)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isLargeBeamLaser)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isShieldRepulser)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSmartShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isExteriorShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isVisibleShield)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isVisibleShieldRepulse)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_getVisibleShieldHitFrames)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse)(int teamId, int weaponDefId, int resourceId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getRadius)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getForce)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getMaxSpeed)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getPower)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getPowerRegen)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource)(int teamId, int weaponDefId, int resourceId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getStartingPower)(int teamId, int weaponDefId);
int (CALLING_CONV *Clb_WeaponDef_Shield_getRechargeDelay)(int teamId, int weaponDefId);
struct SAIFloat3 (CALLING_CONV *Clb_WeaponDef_Shield_getGoodColor)(int teamId, int weaponDefId);
struct SAIFloat3 (CALLING_CONV *Clb_WeaponDef_Shield_getBadColor)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_Shield_getAlpha)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *Clb_WeaponDef_Shield_getInterceptType)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *Clb_WeaponDef_getInterceptedByShieldType)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isAvoidFriendly)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isAvoidFeature)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isAvoidNeutral)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getTargetBorder)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCylinderTargetting)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getMinIntensity)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getHeightBoostFactor)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getProximityPriority)(int teamId, int weaponDefId);
unsigned int (CALLING_CONV *Clb_WeaponDef_getCollisionFlags)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isSweepFire)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isAbleToAttackGround)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getCameraShake)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDynDamageExp)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDynDamageMin)(int teamId, int weaponDefId);
float (CALLING_CONV *Clb_WeaponDef_getDynDamageRange)(int teamId, int weaponDefId);
bool (CALLING_CONV *Clb_WeaponDef_isDynDamageInverted)(int teamId, int weaponDefId);
//const char*[] (CALLING_CONV *Clb_WeaponDef_getCustomParam)(int teamId, int weaponDefId, int index);
//int (CALLING_CONV *Clb_WeaponDef_getCustomParams)(int teamId, int weaponDefId, const char* map[][2]);
int (CALLING_CONV *Clb_WeaponDef_0MAP1SIZE0getCustomParams)(int teamId, int weaponDefId);
void (CALLING_CONV *Clb_WeaponDef_0MAP1KEYS0getCustomParams)(int teamId, int weaponDefId, const char* keys[]);
void (CALLING_CONV *Clb_WeaponDef_0MAP1VALS0getCustomParams)(int teamId, int weaponDefId, const char* values[]);
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

