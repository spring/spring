/*
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
    
#define MAX_AIS 32
    
#include "SAIFloat3.h"


/**
 * Global AI Callback function pointers.
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
int (*handleCommand)(int teamId, int toId, int commandId, int commandTopic, void* commandData);


// BEGINN misc callback functions
int (*Game_getCurrentFrame)(int teamId); //TODO: deprecate, becuase we get the frame from the SUpdateEvent
int (*Game_getAiInterfaceVersion)(int teamId);
int (*Game_getMyTeam)(int teamId);
int (*Game_getMyAllyTeam)(int teamId);
int (*Game_getPlayerTeam)(int teamId, int playerId);
const char* (*Game_getTeamSide)(int teamId, int otherTeamId);
// END misc callback functions

int (*WeaponDef_STATIC_getNumDamageTypes)(int teamId);
unsigned int (*Map_getChecksum)(int teamId);

bool (*Game_isExceptionHandlingEnabled)(int teamId);
bool (*Game_isDebugModeEnabled)(int teamId);
int (*Game_getMode)(int teamId);
bool (*Game_isPaused)(int teamId);
float (*Game_getSpeedFactor)(int teamId);

float (*Gui_getViewRange)(int teamId);
float (*Gui_getScreenX)(int teamId);
float (*Gui_getScreenY)(int teamId);
SAIFloat3 (*Gui_Camera_getDirection)(int teamId);
SAIFloat3 (*Gui_Camera_getPosition)(int teamId);

bool (*File_locateForReading)(int teamId, char* filename);
bool (*File_locateForWriting)(int teamId, char* filename);

int (*Unit_STATIC_getLimit)(int teamId);
const char* (*Game_getSetupScript)(int teamId);


// BEGINN kind of deprecated; it is recommended not to use these
//bool (*getProperty)(int teamId, int id, int property, void* dst);
//bool (*getValue)(int teamId, int id, void* dst);
// END kind of deprecated; it is recommended not to use these


// BEGINN OBJECT Cheats
bool (*Cheats_isEnabled)(int teamId);
bool (*Cheats_setEnabled)(int teamId, bool enable);
bool (*Cheats_setEventsEnabled)(int teamId, bool enabled);
bool (*Cheats_isOnlyPassive)(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT ResourceInfo
float (*ResourceInfo_Metal_getCurrent)(int teamId);
float (*ResourceInfo_Metal_getIncome)(int teamId);
float (*ResourceInfo_Metal_getUsage)(int teamId);
float (*ResourceInfo_Metal_getStorage)(int teamId);
float (*ResourceInfo_Energy_getCurrent)(int teamId);
float (*ResourceInfo_Energy_getIncome)(int teamId);
float (*ResourceInfo_Energy_getUsage)(int teamId);
float (*ResourceInfo_Energy_getStorage)(int teamId);
// END OBJECT ResourceInfo


// BEGINN OBJECT File
int (*File_getSize)(int teamId, const char* fileName);
bool (*File_getContent)(int teamId, const char* filename, void* buffer, int bufferLen);
// END OBJECT File


// BEGINN OBJECT UnitDef
int (*UnitDef_STATIC_getIds)(int teamId, int* unitDefIds);
int (*UnitDef_STATIC_getNumIds)(int teamId);
int (*UnitDef_STATIC_getIdByName)(int teamId, const char* unitName);

float (*UnitDef_getHeight)(int teamId, int unitDefId);
float (*UnitDef_getRadius)(int teamId, int unitDefId);

bool (*UnitDef_isValid)(int teamId, int unitDefId);
const char* (*UnitDef_getName)(int teamId, int unitDefId);
const char* (*UnitDef_getHumanName)(int teamId, int unitDefId);
const char* (*UnitDef_getFilename)(int teamId, int unitDefId);
int (*UnitDef_getId)(int teamId, int unitDefId);
int (*UnitDef_getAiHint)(int teamId, int unitDefId);
int (*UnitDef_getCobID)(int teamId, int unitDefId);
int (*UnitDef_getTechLevel)(int teamId, int unitDefId);
const char* (*UnitDef_getGaia)(int teamId, int unitDefId);
float (*UnitDef_getMetalUpkeep)(int teamId, int unitDefId);
float (*UnitDef_getEnergyUpkeep)(int teamId, int unitDefId);
float (*UnitDef_getMetalMake)(int teamId, int unitDefId);
float (*UnitDef_getMakesMetal)(int teamId, int unitDefId);
float (*UnitDef_getEnergyMake)(int teamId, int unitDefId);
float (*UnitDef_getMetalCost)(int teamId, int unitDefId);
float (*UnitDef_getEnergyCost)(int teamId, int unitDefId);
float (*UnitDef_getBuildTime)(int teamId, int unitDefId);
float (*UnitDef_getExtractsMetal)(int teamId, int unitDefId);
float (*UnitDef_getExtractRange)(int teamId, int unitDefId);
float (*UnitDef_getWindGenerator)(int teamId, int unitDefId);
float (*UnitDef_getTidalGenerator)(int teamId, int unitDefId);
float (*UnitDef_getMetalStorage)(int teamId, int unitDefId);
float (*UnitDef_getEnergyStorage)(int teamId, int unitDefId);
float (*UnitDef_getAutoHeal)(int teamId, int unitDefId);
float (*UnitDef_getIdleAutoHeal)(int teamId, int unitDefId);
int (*UnitDef_getIdleTime)(int teamId, int unitDefId);
float (*UnitDef_getPower)(int teamId, int unitDefId);
float (*UnitDef_getHealth)(int teamId, int unitDefId);
unsigned int (*UnitDef_getCategory)(int teamId, int unitDefId);
float (*UnitDef_getSpeed)(int teamId, int unitDefId);
float (*UnitDef_getTurnRate)(int teamId, int unitDefId);
int (*UnitDef_getMoveType)(int teamId, int unitDefId);
bool (*UnitDef_isUpright)(int teamId, int unitDefId);
bool (*UnitDef_isCollide)(int teamId, int unitDefId);
float (*UnitDef_getControlRadius)(int teamId, int unitDefId);
float (*UnitDef_getLosRadius)(int teamId, int unitDefId);
float (*UnitDef_getAirLosRadius)(int teamId, int unitDefId);
float (*UnitDef_getLosHeight)(int teamId, int unitDefId);
int (*UnitDef_getRadarRadius)(int teamId, int unitDefId);
int (*UnitDef_getSonarRadius)(int teamId, int unitDefId);
int (*UnitDef_getJammerRadius)(int teamId, int unitDefId);
int (*UnitDef_getSonarJamRadius)(int teamId, int unitDefId);
int (*UnitDef_getSeismicRadius)(int teamId, int unitDefId);
float (*UnitDef_getSeismicSignature)(int teamId, int unitDefId);
bool (*UnitDef_isStealth)(int teamId, int unitDefId);
bool (*UnitDef_isSonarStealth)(int teamId, int unitDefId);
bool (*UnitDef_isBuildRange3D)(int teamId, int unitDefId);
float (*UnitDef_getBuildDistance)(int teamId, int unitDefId);
float (*UnitDef_getBuildSpeed)(int teamId, int unitDefId);
float (*UnitDef_getReclaimSpeed)(int teamId, int unitDefId);
float (*UnitDef_getRepairSpeed)(int teamId, int unitDefId);
float (*UnitDef_getMaxRepairSpeed)(int teamId, int unitDefId);
float (*UnitDef_getResurrectSpeed)(int teamId, int unitDefId);
float (*UnitDef_getCaptureSpeed)(int teamId, int unitDefId);
float (*UnitDef_getTerraformSpeed)(int teamId, int unitDefId);
float (*UnitDef_getMass)(int teamId, int unitDefId);
bool (*UnitDef_isPushResistant)(int teamId, int unitDefId);
bool (*UnitDef_isStrafeToAttack)(int teamId, int unitDefId);
float (*UnitDef_getMinCollisionSpeed)(int teamId, int unitDefId);
float (*UnitDef_getSlideTolerance)(int teamId, int unitDefId);
float (*UnitDef_getMaxSlope)(int teamId, int unitDefId);
float (*UnitDef_getMaxHeightDif)(int teamId, int unitDefId);
float (*UnitDef_getMinWaterDepth)(int teamId, int unitDefId);
float (*UnitDef_getWaterline)(int teamId, int unitDefId);
float (*UnitDef_getMaxWaterDepth)(int teamId, int unitDefId);
float (*UnitDef_getArmoredMultiple)(int teamId, int unitDefId);
int (*UnitDef_getArmorType)(int teamId, int unitDefId);
int (*UnitDef_getFlankingBonusMode)(int teamId, int unitDefId);
SAIFloat3 (*UnitDef_getFlankingBonusDir)(int teamId, int unitDefId);
float (*UnitDef_getFlankingBonusMax)(int teamId, int unitDefId);
float (*UnitDef_getFlankingBonusMin)(int teamId, int unitDefId);
float (*UnitDef_getFlankingBonusMobilityAdd)(int teamId, int unitDefId);
const char* (*UnitDef_getCollisionVolumeType)(int teamId, int unitDefId);
SAIFloat3 (*UnitDef_getCollisionVolumeScales)(int teamId, int unitDefId);
SAIFloat3 (*UnitDef_getCollisionVolumeOffsets)(int teamId, int unitDefId);
int (*UnitDef_getCollisionVolumeTest)(int teamId, int unitDefId);
float (*UnitDef_getMaxWeaponRange)(int teamId, int unitDefId);
const char* (*UnitDef_getType)(int teamId, int unitDefId);
const char* (*UnitDef_getTooltip)(int teamId, int unitDefId);
const char* (*UnitDef_getWreckName)(int teamId, int unitDefId);
const char* (*UnitDef_getDeathExplosion)(int teamId, int unitDefId);
const char* (*UnitDef_getSelfDExplosion)(int teamId, int unitDefId);
const char* (*UnitDef_getTedClassString)(int teamId, int unitDefId);
const char* (*UnitDef_getCategoryString)(int teamId, int unitDefId);
bool (*UnitDef_isCanSelfD)(int teamId, int unitDefId);
int (*UnitDef_getSelfDCountdown)(int teamId, int unitDefId);
bool (*UnitDef_isCanSubmerge)(int teamId, int unitDefId);
bool (*UnitDef_isCanFly)(int teamId, int unitDefId);
bool (*UnitDef_isCanMove)(int teamId, int unitDefId);
bool (*UnitDef_isCanHover)(int teamId, int unitDefId);
bool (*UnitDef_isFloater)(int teamId, int unitDefId);
bool (*UnitDef_isBuilder)(int teamId, int unitDefId);
bool (*UnitDef_isActivateWhenBuilt)(int teamId, int unitDefId);
bool (*UnitDef_isOnOffable)(int teamId, int unitDefId);
bool (*UnitDef_isFullHealthFactory)(int teamId, int unitDefId);
bool (*UnitDef_isFactoryHeadingTakeoff)(int teamId, int unitDefId);
bool (*UnitDef_isReclaimable)(int teamId, int unitDefId);
bool (*UnitDef_isCapturable)(int teamId, int unitDefId);
bool (*UnitDef_isCanRestore)(int teamId, int unitDefId);
bool (*UnitDef_isCanRepair)(int teamId, int unitDefId);
bool (*UnitDef_isCanSelfRepair)(int teamId, int unitDefId);
bool (*UnitDef_isCanReclaim)(int teamId, int unitDefId);
bool (*UnitDef_isCanAttack)(int teamId, int unitDefId);
bool (*UnitDef_isCanPatrol)(int teamId, int unitDefId);
bool (*UnitDef_isCanFight)(int teamId, int unitDefId);
bool (*UnitDef_isCanGuard)(int teamId, int unitDefId);
bool (*UnitDef_isCanBuild)(int teamId, int unitDefId);
bool (*UnitDef_isCanAssist)(int teamId, int unitDefId);
bool (*UnitDef_isCanBeAssisted)(int teamId, int unitDefId);
bool (*UnitDef_isCanRepeat)(int teamId, int unitDefId);
bool (*UnitDef_isCanFireControl)(int teamId, int unitDefId);
int (*UnitDef_getFireState)(int teamId, int unitDefId);
int (*UnitDef_getMoveState)(int teamId, int unitDefId);
float (*UnitDef_getWingDrag)(int teamId, int unitDefId);
float (*UnitDef_getWingAngle)(int teamId, int unitDefId);
float (*UnitDef_getDrag)(int teamId, int unitDefId);
float (*UnitDef_getFrontToSpeed)(int teamId, int unitDefId);
float (*UnitDef_getSpeedToFront)(int teamId, int unitDefId);
float (*UnitDef_getMyGravity)(int teamId, int unitDefId);
float (*UnitDef_getMaxBank)(int teamId, int unitDefId);
float (*UnitDef_getMaxPitch)(int teamId, int unitDefId);
float (*UnitDef_getTurnRadius)(int teamId, int unitDefId);
float (*UnitDef_getWantedHeight)(int teamId, int unitDefId);
float (*UnitDef_getVerticalSpeed)(int teamId, int unitDefId);
bool (*UnitDef_isCanCrash)(int teamId, int unitDefId);
bool (*UnitDef_isHoverAttack)(int teamId, int unitDefId);
bool (*UnitDef_isAirStrafe)(int teamId, int unitDefId);
float (*UnitDef_getDlHoverFactor)(int teamId, int unitDefId);
float (*UnitDef_getMaxAcceleration)(int teamId, int unitDefId);
float (*UnitDef_getMaxDeceleration)(int teamId, int unitDefId);
float (*UnitDef_getMaxAileron)(int teamId, int unitDefId);
float (*UnitDef_getMaxElevator)(int teamId, int unitDefId);
float (*UnitDef_getMaxRudder)(int teamId, int unitDefId);
///* returned size is 4 */
//const unsigned char** (*UnitDef_getYardMaps)(int teamId, int unitDefId);
int (*UnitDef_getXSize)(int teamId, int unitDefId);
int (*UnitDef_getYSize)(int teamId, int unitDefId);
int (*UnitDef_getBuildAngle)(int teamId, int unitDefId);
float (*UnitDef_getLoadingRadius)(int teamId, int unitDefId);
float (*UnitDef_getUnloadSpread)(int teamId, int unitDefId);
int (*UnitDef_getTransportCapacity)(int teamId, int unitDefId);
int (*UnitDef_getTransportSize)(int teamId, int unitDefId);
int (*UnitDef_getMinTransportSize)(int teamId, int unitDefId);
bool (*UnitDef_isAirBase)(int teamId, int unitDefId);
float (*UnitDef_getTransportMass)(int teamId, int unitDefId);
float (*UnitDef_getMinTransportMass)(int teamId, int unitDefId);
bool (*UnitDef_isHoldSteady)(int teamId, int unitDefId);
bool (*UnitDef_isReleaseHeld)(int teamId, int unitDefId);
bool (*UnitDef_isCantBeTransported)(int teamId, int unitDefId);
bool (*UnitDef_isTransportByEnemy)(int teamId, int unitDefId);
int (*UnitDef_getTransportUnloadMethod)(int teamId, int unitDefId);
float (*UnitDef_getFallSpeed)(int teamId, int unitDefId);
float (*UnitDef_getUnitFallSpeed)(int teamId, int unitDefId);
bool (*UnitDef_isCanCloak)(int teamId, int unitDefId);
bool (*UnitDef_isStartCloaked)(int teamId, int unitDefId);
float (*UnitDef_getCloakCost)(int teamId, int unitDefId);
float (*UnitDef_getCloakCostMoving)(int teamId, int unitDefId);
float (*UnitDef_getDecloakDistance)(int teamId, int unitDefId);
bool (*UnitDef_isDecloakSpherical)(int teamId, int unitDefId);
bool (*UnitDef_isDecloakOnFire)(int teamId, int unitDefId);
bool (*UnitDef_isCanKamikaze)(int teamId, int unitDefId);
float (*UnitDef_getKamikazeDist)(int teamId, int unitDefId);
bool (*UnitDef_isTargetingFacility)(int teamId, int unitDefId);
bool (*UnitDef_isCanDGun)(int teamId, int unitDefId);
bool (*UnitDef_isNeedGeo)(int teamId, int unitDefId);
bool (*UnitDef_isFeature)(int teamId, int unitDefId);
bool (*UnitDef_isHideDamage)(int teamId, int unitDefId);
bool (*UnitDef_isCommander)(int teamId, int unitDefId);
bool (*UnitDef_isShowPlayerName)(int teamId, int unitDefId);
bool (*UnitDef_isCanResurrect)(int teamId, int unitDefId);
bool (*UnitDef_isCanCapture)(int teamId, int unitDefId);
int (*UnitDef_getHighTrajectoryType)(int teamId, int unitDefId);
unsigned int (*UnitDef_getNoChaseCategory)(int teamId, int unitDefId);
bool (*UnitDef_isLeaveTracks)(int teamId, int unitDefId);
float (*UnitDef_getTrackWidth)(int teamId, int unitDefId);
float (*UnitDef_getTrackOffset)(int teamId, int unitDefId);
float (*UnitDef_getTrackStrength)(int teamId, int unitDefId);
float (*UnitDef_getTrackStretch)(int teamId, int unitDefId);
int (*UnitDef_getTrackType)(int teamId, int unitDefId);
bool (*UnitDef_isCanDropFlare)(int teamId, int unitDefId);
float (*UnitDef_getFlareReloadTime)(int teamId, int unitDefId);
float (*UnitDef_getFlareEfficiency)(int teamId, int unitDefId);
float (*UnitDef_getFlareDelay)(int teamId, int unitDefId);
SAIFloat3 (*UnitDef_getFlareDropVector)(int teamId, int unitDefId);
int (*UnitDef_getFlareTime)(int teamId, int unitDefId);
int (*UnitDef_getFlareSalvoSize)(int teamId, int unitDefId);
int (*UnitDef_getFlareSalvoDelay)(int teamId, int unitDefId);
bool (*UnitDef_isSmoothAnim)(int teamId, int unitDefId);
bool (*UnitDef_isMetalMaker)(int teamId, int unitDefId);
bool (*UnitDef_isCanLoopbackAttack)(int teamId, int unitDefId);
bool (*UnitDef_isLevelGround)(int teamId, int unitDefId);
bool (*UnitDef_isUseBuildingGroundDecal)(int teamId, int unitDefId);
int (*UnitDef_getBuildingDecalType)(int teamId, int unitDefId);
int (*UnitDef_getBuildingDecalSizeX)(int teamId, int unitDefId);
int (*UnitDef_getBuildingDecalSizeY)(int teamId, int unitDefId);
float (*UnitDef_getBuildingDecalDecaySpeed)(int teamId, int unitDefId);
bool (*UnitDef_isFirePlatform)(int teamId, int unitDefId);
float (*UnitDef_getMaxFuel)(int teamId, int unitDefId);
float (*UnitDef_getRefuelTime)(int teamId, int unitDefId);
float (*UnitDef_getMinAirBasePower)(int teamId, int unitDefId);
int (*UnitDef_getMaxThisUnit)(int teamId, int unitDefId);
int (*UnitDef_getDecoyDefId)(int teamId, int unitDefId);
bool (*UnitDef_isDontLand)(int teamId, int unitDefId);
int (*UnitDef_getShieldWeaponDefId)(int teamId, int unitDefId);
int (*UnitDef_getStockpileWeaponDefId)(int teamId, int unitDefId);
int (*UnitDef_getNumBuildOptions)(int teamId, int unitDefId);
int (*UnitDef_getBuildOptions)(int teamId, int unitDefId, int* unitDefIds);
int (*UnitDef_getNumCustomParams)(int teamId, int unitDefId);
int (*UnitDef_getCustomParams)(int teamId, int unitDefId, const char* map[][2]);
bool (*UnitDef_hasMoveData)(int teamId, int unitDefId);
/* enum MoveType { Ground_Move, Hover_Move, Ship_Move }; */
int (*UnitDef_MoveData_getMoveType)(int teamId, int unitDefId);
/* 0=tank,1=kbot,2=hover,3=ship */
int (*UnitDef_MoveData_getMoveFamily)(int teamId, int unitDefId);
int (*UnitDef_MoveData_getSize)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getDepth)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getMaxSlope)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getSlopeMod)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getDepthMod)(int teamId, int unitDefId);
int (*UnitDef_MoveData_getPathType)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getCrushStrength)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getMaxSpeed)(int teamId, int unitDefId);
short (*UnitDef_MoveData_getMaxTurnRate)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getMaxAcceleration)(int teamId, int unitDefId);
float (*UnitDef_MoveData_getMaxBreaking)(int teamId, int unitDefId);
bool (*UnitDef_MoveData_isSubMarine)(int teamId, int unitDefId);
int (*UnitDef_getNumUnitDefWeapons)(int teamId, int unitDefId);
const char* (*UnitDef_UnitDefWeapon_getName)(int teamId, int unitDefId, int weaponIndex);
int (*UnitDef_UnitDefWeapon_getWeaponDefId)(int teamId, int unitDefId, int weaponIndex);
int (*UnitDef_UnitDefWeapon_getSlavedTo)(int teamId, int unitDefId, int weaponIndex);
SAIFloat3 (*UnitDef_UnitDefWeapon_getMainDir)(int teamId, int unitDefId, int weaponIndex);
float (*UnitDef_UnitDefWeapon_getMaxAngleDif)(int teamId, int unitDefId, int weaponIndex);
float (*UnitDef_UnitDefWeapon_getFuelUsage)(int teamId, int unitDefId, int weaponIndex);
unsigned int (*UnitDef_UnitDefWeapon_getBadTargetCat)(int teamId, int unitDefId, int weaponIndex);
unsigned int (*UnitDef_UnitDefWeapon_getOnlyTargetCat)(int teamId, int unitDefId, int weaponIndex);
// END OBJECT UnitDef



// BEGINN OBJECT Unit
int (*Unit_STATIC_getEnemies)(int teamId, int* unitIds);
int (*Unit_STATIC_getEnemiesIn)(int teamId, int* unitIds, SAIFloat3 pos, float radius);
int (*Unit_STATIC_getEnemiesInRadarAndLos)(int teamId, int* units);
int (*Unit_STATIC_getFriendlies)(int teamId, int* unitIds);
int (*Unit_STATIC_getFriendliesIn)(int teamId, int* unitIds, SAIFloat3 pos, float radius);
int (*Unit_STATIC_getNeutrals)(int teamId, int* unitIds);
int (*Unit_STATIC_getNeutralsIn)(int teamId, int* unitIds, SAIFloat3 pos, float radius);
int (*Unit_STATIC_getSelected)(int teamId, int* unitIds);

int (*Unit_getDefId)(int teamId, int unitId);
int (*Unit_getAiHint)(int teamId, int unitId);
int (*Unit_getTeam)(int teamId, int unitId);
int (*Unit_getAllyTeam)(int teamId, int unitId);

int (*Unit_getStockpile)(int teamId, int unitId);
int (*Unit_getStockpileQueued)(int teamId, int unitId);
float (*Unit_getCurrentFuel)(int teamId, int unitId);
float (*Unit_getMaxSpeed)(int teamId, int unitId);
float (*Unit_getMaxRange)(int teamId, int unitId);
float (*Unit_getMaxHealth)(int teamId, int unitId);
float (*Unit_getExperience)(int teamId, int unitId);
int (*Unit_getGroup)(int teamId, int unitId);
int (*Unit_getNumCurrentCommands)(int teamId, int unitId);
/* for the type of the command queue, see CCommandQueue::CommandQueueType CommandQueue.h */
int (*Unit_CurrentCommands_getType)(int teamId, int unitId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (*Unit_CurrentCommands_getIds)(int teamId, int unitId, int* ids);
int (*Unit_CurrentCommands_getOptions)(int teamId, int unitId, unsigned char* options);
int (*Unit_CurrentCommands_getTag)(int teamId, int unitId, unsigned int* tags);
int (*Unit_CurrentCommands_getTimeOut)(int teamId, int unitId, int* timeOuts);
int (*Unit_CurrentCommands_getNumParams)(int teamId, int unitId, int* numParams);
int (*Unit_CurrentCommands_getParams)(int teamId, int unitId, float** params);
int (*Unit_getNumSupportedCommands)(int teamId, int unitId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (*Unit_SupportedCommands_getId)(int teamId, int unitId, int* ids);
int (*Unit_SupportedCommands_getName)(int teamId, int unitId, const char** names);
int (*Unit_SupportedCommands_getToolTip)(int teamId, int unitId, const char** toolTips);
int (*Unit_SupportedCommands_isShowUnique)(int teamId, int unitId, bool* showUniques);
int (*Unit_SupportedCommands_isDisabled)(int teamId, int unitId, bool* disableds);
int (*Unit_SupportedCommands_getNumParams)(int teamId, int unitId, int* numParams);
int (*Unit_SupportedCommands_getParams)(int teamId, int unitId, const char*** params);
float (*Unit_getHealth)(int teamId, int unitId);
float (*Unit_getSpeed)(int teamId, int unitId);
float (*Unit_getPower)(int teamId, int unitId);
float (*Unit_ResourceInfo_Metal_getUse)(int teamId, int unitId);
float (*Unit_ResourceInfo_Metal_getMake)(int teamId, int unitId);
float (*Unit_ResourceInfo_Energy_getUse)(int teamId, int unitId);
float (*Unit_ResourceInfo_Energy_getMake)(int teamId, int unitId);
SAIFloat3 (*Unit_getPos)(int teamId, int unitId);
bool (*Unit_isActivated)(int teamId, int unitId);
bool (*Unit_isBeingBuilt)(int teamId, int unitId);
bool (*Unit_isCloaked)(int teamId, int unitId);
bool (*Unit_isParalyzed)(int teamId, int unitId);
bool (*Unit_isNeutral)(int teamId, int unitId);
int (*Unit_getBuildingFacing)(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
int (*Group_getNumSupportedCommands)(int teamId, int groupId);
/* for the id, see CMD_xxx codes in Command.h  (custom codes can also be used) */
int (*Group_SupportedCommands_getId)(int teamId, int groupId, int* ids);
int (*Group_SupportedCommands_getName)(int teamId, int groupId, const char** names);
int (*Group_SupportedCommands_getToolTip)(int teamId, int groupId, const char** toolTips);
int (*Group_SupportedCommands_isShowUnique)(int teamId, int groupId, bool* showUniques);
int (*Group_SupportedCommands_isDisabled)(int teamId, int groupId, bool* disableds);
int (*Group_SupportedCommands_getNumParams)(int teamId, int groupId, int* numParams);
int (*Group_SupportedCommands_getParams)(int teamId, int groupId, const char*** params);
// END OBJECT Group



// BEGINN OBJECT Mod
const char* (*Mod_getName)(int teamId);
// END OBJECT Mod



// BEGINN OBJECT Map
SAIFloat3 (*Map_getStartPos)(int teamId);
SAIFloat3 (*Map_getMousePos)(int teamId);
bool (*Map_isPosInCamera)(int teamId, SAIFloat3 pos, float radius);
int (*Map_getWidth)(int teamId);
int (*Map_getHeight)(int teamId);
const float* (*Map_getHeightMap)(int teamId);
float (*Map_getMinHeight)(int teamId);
float (*Map_getMaxHeight)(int teamId);
const float* (*Map_getSlopeMap)(int teamId);
const unsigned short* (*Map_getLosMap)(int teamId);
const unsigned short* (*Map_getRadarMap)(int teamId);
const unsigned short* (*Map_getJammerMap)(int teamId);
const unsigned char* (*Map_getMetalMap)(int teamId);
const char* (*Map_getName)(int teamId);
float (*Map_getElevationAt)(int teamId, float x, float z);
float (*Map_getMaxMetal)(int teamId);
float (*Map_getExtractorRadius)(int teamId);
float (*Map_getMinWind)(int teamId);
float (*Map_getMaxWind)(int teamId);
float (*Map_getTidalStrength)(int teamId);
float (*Map_getGravity)(int teamId);
int (*Map_getPoints)(int teamId, SAIFloat3* positions, unsigned char** colors, const char** labels, int maxPoints);
int (*Map_getLines)(int teamId, SAIFloat3* firstPositions, SAIFloat3* secondPositions, unsigned char** colors, int maxLines);
bool (*Map_canBuildAt)(int teamId, int unitDefId, SAIFloat3 pos, int facing);
SAIFloat3 (*Map_findClosestBuildSite)(int teamId, int unitDefId, SAIFloat3 pos, float searchRadius, int minDist, int facing);
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
//const SAIFeatureDef* (*getFeatureDef)(int teamId, int featureDefId);
const char* (*FeatureDef_getName)(int teamId, int weaponDefId);
const char* (*FeatureDef_getDescription)(int teamId, int weaponDefId);
const char* (*FeatureDef_getFilename)(int teamId, int weaponDefId);
int (*FeatureDef_getId)(int teamId, int weaponDefId);
float (*FeatureDef_getMetal)(int teamId, int weaponDefId);
float (*FeatureDef_getEnergy)(int teamId, int weaponDefId);
float (*FeatureDef_getMaxHealth)(int teamId, int weaponDefId);
float (*FeatureDef_getReclaimTime)(int teamId, int weaponDefId);
float (*FeatureDef_getMass)(int teamId, int weaponDefId);
const char* (*FeatureDef_getCollisionVolumeType)(int teamId, int weaponDefId);	
SAIFloat3 (*FeatureDef_getCollisionVolumeScales)(int teamId, int weaponDefId);		
SAIFloat3 (*FeatureDef_getCollisionVolumeOffsets)(int teamId, int weaponDefId);		
int (*FeatureDef_getCollisionVolumeTest)(int teamId, int weaponDefId);			
bool (*FeatureDef_isUpright)(int teamId, int weaponDefId);
int (*FeatureDef_getDrawType)(int teamId, int weaponDefId);
const char* (*FeatureDef_getModelName)(int teamId, int weaponDefId);
int (*FeatureDef_getModelType)(int teamId, int weaponDefId);
bool (*FeatureDef_isDestructable)(int teamId, int weaponDefId);
bool (*FeatureDef_isReclaimable)(int teamId, int weaponDefId);
bool (*FeatureDef_isBlocking)(int teamId, int weaponDefId);
bool (*FeatureDef_isBurnable)(int teamId, int weaponDefId);
bool (*FeatureDef_isFloating)(int teamId, int weaponDefId);
bool (*FeatureDef_isNoSelect)(int teamId, int weaponDefId);
bool (*FeatureDef_isGeoThermal)(int teamId, int weaponDefId);
const char* (*FeatureDef_getDeathFeature)(int teamId, int weaponDefId);
int (*FeatureDef_getXsize)(int teamId, int weaponDefId);
int (*FeatureDef_getYsize)(int teamId, int weaponDefId);
int (*FeatureDef_getNumCustomParams)(int teamId, int weaponDefId);
int (*FeatureDef_getCustomParams)(int teamId, int weaponDefId, const char* map[][2]);
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
int (*Feature_STATIC_getIds)(int teamId, int *featureIds, int max);
int (*Feature_STATIC_getIdsIn)(int teamId, int *featureIds, int max, SAIFloat3 pos, float radius);

int (*Feature_getDefId)(int teamId, int featureId);
float (*Feature_getHealth)(int teamId, int featureId);
float (*Feature_getReclaimLeft)(int teamId, int featureId);
SAIFloat3 (*Feature_getPos)(int teamId, int featureId);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
int (*WeaponDef_STATIC_getIdByName)(int teamId, const char* weaponDefName);

//const SAIWeaponDef* (*getWeaponDef)(int teamId, int weaponDefId);
const char* (*WeaponDef_getName)(int teamId, int weaponDefId);
const char* (*WeaponDef_getType)(int teamId, int weaponDefId);
const char* (*WeaponDef_getDescription)(int teamId, int weaponDefId);
const char* (*WeaponDef_getFilename)(int teamId, int weaponDefId);
const char* (*WeaponDef_getCegTag)(int teamId, int weaponDefId);
float (*WeaponDef_getRange)(int teamId, int weaponDefId);
float (*WeaponDef_getHeightMod)(int teamId, int weaponDefId);
float (*WeaponDef_getAccuracy)(int teamId, int weaponDefId);
float (*WeaponDef_getSprayAngle)(int teamId, int weaponDefId);
float (*WeaponDef_getMovingAccuracy)(int teamId, int weaponDefId);
float (*WeaponDef_getTargetMoveError)(int teamId, int weaponDefId);
float (*WeaponDef_getLeadLimit)(int teamId, int weaponDefId);
float (*WeaponDef_getLeadBonus)(int teamId, int weaponDefId);
float (*WeaponDef_getPredictBoost)(int teamId, int weaponDefId);

//DamageArray (*WeaponDef_getDamages)(int teamId, int weaponDefId);
int (*WeaponDef_Damages_getParalyzeDamageTime)(int teamId, int weaponDefId);
float (*WeaponDef_Damages_getImpulseFactor)(int teamId, int weaponDefId);
float (*WeaponDef_Damages_getImpulseBoost)(int teamId, int weaponDefId);
float (*WeaponDef_Damages_getCraterMult)(int teamId, int weaponDefId);
float (*WeaponDef_Damages_getCraterBoost)(int teamId, int weaponDefId);
int (*WeaponDef_Damages_getNumTypes)(int teamId, int weaponDefId);
//float (*WeaponDef_Damages_getType)(int teamId, int weaponDefId, int typeIndex);
void (*WeaponDef_Damages_getTypeDamages)(int teamId, int weaponDefId, float* typeDamages);

float (*WeaponDef_getAreaOfEffect)(int teamId, int weaponDefId);
bool (*WeaponDef_isNoSelfDamage)(int teamId, int weaponDefId);
float (*WeaponDef_getFireStarter)(int teamId, int weaponDefId);
float (*WeaponDef_getEdgeEffectiveness)(int teamId, int weaponDefId);
float (*WeaponDef_getSize)(int teamId, int weaponDefId);
float (*WeaponDef_getSizeGrowth)(int teamId, int weaponDefId);
float (*WeaponDef_getCollisionSize)(int teamId, int weaponDefId);
int (*WeaponDef_getSalvoSize)(int teamId, int weaponDefId);
float (*WeaponDef_getSalvoDelay)(int teamId, int weaponDefId);
float (*WeaponDef_getReload)(int teamId, int weaponDefId);
float (*WeaponDef_getBeamTime)(int teamId, int weaponDefId);
bool (*WeaponDef_isBeamBurst)(int teamId, int weaponDefId);
bool (*WeaponDef_isWaterBounce)(int teamId, int weaponDefId);
bool (*WeaponDef_isGroundBounce)(int teamId, int weaponDefId);
float (*WeaponDef_getBounceRebound)(int teamId, int weaponDefId);
float (*WeaponDef_getBounceSlip)(int teamId, int weaponDefId);
int (*WeaponDef_getNumBounce)(int teamId, int weaponDefId);
float (*WeaponDef_getMaxAngle)(int teamId, int weaponDefId);
float (*WeaponDef_getRestTime)(int teamId, int weaponDefId);
float (*WeaponDef_getUpTime)(int teamId, int weaponDefId);
int (*WeaponDef_getFlightTime)(int teamId, int weaponDefId);
float (*WeaponDef_getMetalCost)(int teamId, int weaponDefId);
float (*WeaponDef_getEnergyCost)(int teamId, int weaponDefId);
float (*WeaponDef_getSupplyCost)(int teamId, int weaponDefId);
int (*WeaponDef_getProjectilesPerShot)(int teamId, int weaponDefId);
int (*WeaponDef_getId)(int teamId, int weaponDefId);
int (*WeaponDef_getTdfId)(int teamId, int weaponDefId);
bool (*WeaponDef_isTurret)(int teamId, int weaponDefId);
bool (*WeaponDef_isOnlyForward)(int teamId, int weaponDefId);
bool (*WeaponDef_isFixedLauncher)(int teamId, int weaponDefId);
bool (*WeaponDef_isWaterWeapon)(int teamId, int weaponDefId);
bool (*WeaponDef_isFireSubmersed)(int teamId, int weaponDefId);
bool (*WeaponDef_isSubMissile)(int teamId, int weaponDefId);
bool (*WeaponDef_isTracks)(int teamId, int weaponDefId);
bool (*WeaponDef_isDropped)(int teamId, int weaponDefId);
bool (*WeaponDef_isParalyzer)(int teamId, int weaponDefId);
bool (*WeaponDef_isImpactOnly)(int teamId, int weaponDefId);
bool (*WeaponDef_isNoAutoTarget)(int teamId, int weaponDefId);
bool (*WeaponDef_isManualFire)(int teamId, int weaponDefId);
int (*WeaponDef_getInterceptor)(int teamId, int weaponDefId);
int (*WeaponDef_getTargetable)(int teamId, int weaponDefId);
bool (*WeaponDef_isStockpileable)(int teamId, int weaponDefId);
float (*WeaponDef_getCoverageRange)(int teamId, int weaponDefId);
float (*WeaponDef_getIntensity)(int teamId, int weaponDefId);
float (*WeaponDef_getThickness)(int teamId, int weaponDefId);
float (*WeaponDef_getLaserFlareSize)(int teamId, int weaponDefId);
float (*WeaponDef_getCoreThickness)(int teamId, int weaponDefId);
float (*WeaponDef_getDuration)(int teamId, int weaponDefId);
int (*WeaponDef_getLodDistance)(int teamId, int weaponDefId);
float (*WeaponDef_getFalloffRate)(int teamId, int weaponDefId);
int (*WeaponDef_getGraphicsType)(int teamId, int weaponDefId);
bool (*WeaponDef_isSoundTrigger)(int teamId, int weaponDefId);
bool (*WeaponDef_isSelfExplode)(int teamId, int weaponDefId);
bool (*WeaponDef_isGravityAffected)(int teamId, int weaponDefId);
int (*WeaponDef_getHighTrajectory)(int teamId, int weaponDefId);
float (*WeaponDef_getMyGravity)(int teamId, int weaponDefId);
bool (*WeaponDef_isTwoPhase)(int teamId, int weaponDefId);
bool (*WeaponDef_isGuided)(int teamId, int weaponDefId);
bool (*WeaponDef_isVLaunched)(int teamId, int weaponDefId);
bool (*WeaponDef_isSelfPropelled)(int teamId, int weaponDefId);
bool (*WeaponDef_isNoExplode)(int teamId, int weaponDefId);
float (*WeaponDef_getStartVelocity)(int teamId, int weaponDefId);
float (*WeaponDef_getWeaponAcceleration)(int teamId, int weaponDefId);
float (*WeaponDef_getTurnRate)(int teamId, int weaponDefId);
float (*WeaponDef_getMaxVelocity)(int teamId, int weaponDefId);
float (*WeaponDef_getProjectileSpeed)(int teamId, int weaponDefId);
float (*WeaponDef_getExplosionSpeed)(int teamId, int weaponDefId);
unsigned int (*WeaponDef_getOnlyTargetCategory)(int teamId, int weaponDefId);
float (*WeaponDef_getWobble)(int teamId, int weaponDefId);
float (*WeaponDef_getDance)(int teamId, int weaponDefId);
float (*WeaponDef_getTrajectoryHeight)(int teamId, int weaponDefId);
bool (*WeaponDef_isLargeBeamLaser)(int teamId, int weaponDefId);
bool (*WeaponDef_isShield)(int teamId, int weaponDefId);
bool (*WeaponDef_isShieldRepulser)(int teamId, int weaponDefId);
bool (*WeaponDef_isSmartShield)(int teamId, int weaponDefId);
bool (*WeaponDef_isExteriorShield)(int teamId, int weaponDefId);
bool (*WeaponDef_isVisibleShield)(int teamId, int weaponDefId);
bool (*WeaponDef_isVisibleShieldRepulse)(int teamId, int weaponDefId);
int (*WeaponDef_getVisibleShieldHitFrames)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldEnergyUse)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldRadius)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldForce)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldMaxSpeed)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldPower)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldPowerRegen)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldPowerRegenEnergy)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldStartingPower)(int teamId, int weaponDefId);
int (*WeaponDef_getShieldRechargeDelay)(int teamId, int weaponDefId);
SAIFloat3 (*WeaponDef_getShieldGoodColor)(int teamId, int weaponDefId);
SAIFloat3 (*WeaponDef_getShieldBadColor)(int teamId, int weaponDefId);
float (*WeaponDef_getShieldAlpha)(int teamId, int weaponDefId);
unsigned int (*WeaponDef_getShieldInterceptType)(int teamId, int weaponDefId);
unsigned int (*WeaponDef_getInterceptedByShieldType)(int teamId, int weaponDefId);
bool (*WeaponDef_isAvoidFriendly)(int teamId, int weaponDefId);
bool (*WeaponDef_isAvoidFeature)(int teamId, int weaponDefId);
bool (*WeaponDef_isAvoidNeutral)(int teamId, int weaponDefId);
float (*WeaponDef_getTargetBorder)(int teamId, int weaponDefId);
float (*WeaponDef_getCylinderTargetting)(int teamId, int weaponDefId);
float (*WeaponDef_getMinIntensity)(int teamId, int weaponDefId);
float (*WeaponDef_getHeightBoostFactor)(int teamId, int weaponDefId);
float (*WeaponDef_getProximityPriority)(int teamId, int weaponDefId);
unsigned int (*WeaponDef_getCollisionFlags)(int teamId, int weaponDefId);
bool (*WeaponDef_isSweepFire)(int teamId, int weaponDefId);
bool (*WeaponDef_isCanAttackGround)(int teamId, int weaponDefId);
float (*WeaponDef_getCameraShake)(int teamId, int weaponDefId);
float (*WeaponDef_getDynDamageExp)(int teamId, int weaponDefId);
float (*WeaponDef_getDynDamageMin)(int teamId, int weaponDefId);
float (*WeaponDef_getDynDamageRange)(int teamId, int weaponDefId);
bool (*WeaponDef_isDynDamageInverted)(int teamId, int weaponDefId);
int (*WeaponDef_getNumCustomParams)(int teamId, int weaponDefId);
//const char** (*WeaponDef_getCustomParam)(int teamId, int weaponDefId, int index);
int (*WeaponDef_getCustomParams)(int teamId, int weaponDefId, const char* map[][2]);
// END OBJECT WeaponDef

};

#ifdef	__cplusplus
}
#endif

#endif	/* _SAICALLBACK_H */

