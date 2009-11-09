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

#ifndef _SSKIRMISHAICALLBACKIMPL_H
#define	_SSKIRMISHAICALLBACKIMPL_H

// Doc-comments for the functions in this header can be found in this file
#include "Interface/SSkirmishAICallback.h"

#if	defined(__cplusplus)
extern "C" {
#endif

EXPORT(int              ) skirmishAiCallback_Engine_handleCommand(int teamId, int toId, int commandId, int commandTopic, void* commandData);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getMajor(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getMinor(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getPatchset(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getAdditional(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getBuildTime(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getNormal(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getFull(int teamId);


EXPORT(int              ) skirmishAiCallback_Teams_getSize(int teamId);

EXPORT(int              ) skirmishAiCallback_SkirmishAIs_getSize(int teamId);
EXPORT(int              ) skirmishAiCallback_SkirmishAIs_getMax(int teamId);

EXPORT(int              ) skirmishAiCallback_SkirmishAI_Info_getSize(int teamId);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getKey(int teamId, int infoIndex);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getValue(int teamId, int infoIndex);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getDescription(int teamId, int infoIndex);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getValueByKey(int teamId, const char* const key);

EXPORT(int              ) skirmishAiCallback_SkirmishAI_OptionValues_getSize(int teamId);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_OptionValues_getKey(int teamId, int optionIndex);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_OptionValues_getValue(int teamId, int optionIndex);
EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey(int teamId, const char* const key);

EXPORT(void             ) skirmishAiCallback_Log_log(int teamId, const char* const msg);
EXPORT(void             ) skirmishAiCallback_Log_exception(int teamId, const char* const msg, int severety, bool die);

EXPORT(char             ) skirmishAiCallback_DataDirs_getPathSeparator(int UNUSED_teamId);
EXPORT(int              ) skirmishAiCallback_DataDirs_Roots_getSize(int UNUSED_teamId);
EXPORT(bool             ) skirmishAiCallback_DataDirs_Roots_getDir(int UNUSED_teamId, char* path, int path_sizeMax, int dirIndex);
EXPORT(bool             ) skirmishAiCallback_DataDirs_Roots_locatePath(int UNUSED_teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(char*            ) skirmishAiCallback_DataDirs_Roots_allocatePath(int UNUSED_teamId, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(const char*      ) skirmishAiCallback_DataDirs_getConfigDir(int teamId);
EXPORT(bool             ) skirmishAiCallback_DataDirs_locatePath(int teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);
EXPORT(char*            ) skirmishAiCallback_DataDirs_allocatePath(int teamId, const char* const relPath, bool writeable, bool create, bool dir, bool common);
EXPORT(const char*      ) skirmishAiCallback_DataDirs_getWriteableDir(int teamId);

// BEGINN misc callback functions
EXPORT(int              ) skirmishAiCallback_Game_getCurrentFrame(int teamId);
EXPORT(int              ) skirmishAiCallback_Game_getAiInterfaceVersion(int teamId);
EXPORT(int              ) skirmishAiCallback_Game_getMyTeam(int teamId);
EXPORT(int              ) skirmishAiCallback_Game_getMyAllyTeam(int teamId);
EXPORT(int              ) skirmishAiCallback_Game_getPlayerTeam(int teamId, int playerId);
EXPORT(const char*      ) skirmishAiCallback_Game_getTeamSide(int teamId, int otherTeamId);
EXPORT(bool             ) skirmishAiCallback_Game_isExceptionHandlingEnabled(int teamId);
EXPORT(bool             ) skirmishAiCallback_Game_isDebugModeEnabled(int teamId);
EXPORT(int              ) skirmishAiCallback_Game_getMode(int teamId);
EXPORT(bool             ) skirmishAiCallback_Game_isPaused(int teamId);
EXPORT(float            ) skirmishAiCallback_Game_getSpeedFactor(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Game_getSetupScript(int teamId);
// END misc callback functions


// BEGINN Visualization related callback functions
EXPORT(float            ) skirmishAiCallback_Gui_getViewRange(int teamId);
EXPORT(float            ) skirmishAiCallback_Gui_getScreenX(int teamId);
EXPORT(float            ) skirmishAiCallback_Gui_getScreenY(int teamId);
EXPORT(void             ) skirmishAiCallback_Gui_Camera_getDirection(int teamId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_Gui_Camera_getPosition(int teamId, float* return_posF3_out);
// END Visualization related callback functions


// BEGINN OBJECT Cheats
EXPORT(bool             ) skirmishAiCallback_Cheats_isEnabled(int teamId);
EXPORT(bool             ) skirmishAiCallback_Cheats_setEnabled(int teamId, bool enable);
EXPORT(bool             ) skirmishAiCallback_Cheats_setEventsEnabled(int teamId, bool enabled);
EXPORT(bool             ) skirmishAiCallback_Cheats_isOnlyPassive(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT Resource
EXPORT(int              ) skirmishAiCallback_getResources(int teamId);
EXPORT(int              ) skirmishAiCallback_getResourceByName(int teamId, const char* resourceName);
EXPORT(const char*      ) skirmishAiCallback_Resource_getName(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Resource_getOptimum(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_getCurrent(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_getIncome(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_getUsage(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_getStorage(int teamId, int resourceId);
// END OBJECT Resource


// BEGINN OBJECT File
EXPORT(int              ) skirmishAiCallback_File_getSize(int teamId, const char* fileName);
EXPORT(bool             ) skirmishAiCallback_File_getContent(int teamId, const char* fileName, void* buffer, int bufferLen);
// EXPORT(bool             ) skirmishAiCallback_File_locateForReading(int teamId, char* fileName, int fileName_sizeMax);
// EXPORT(bool             ) skirmishAiCallback_File_locateForWriting(int teamId, char* fileName, int fileName_sizeMax);
// END OBJECT File


// BEGINN OBJECT UnitDef
EXPORT(int              ) skirmishAiCallback_getUnitDefs(int teamId, int* unitDefIds, int unitDefIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getUnitDefByName(int teamId, const char* unitName);
//int) skirmishAiCallback_UnitDef_getId(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getHeight(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getRadius(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isValid(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getName(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getHumanName(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getFileName(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getAiHint(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getCobId(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getTechLevel(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getGaia(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getUpkeep(int teamId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getResourceMake(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMakesResource(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getCost(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getExtractsResource(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getResourceExtractorRange(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getWindResourceGenerator(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTidalResourceGenerator(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getStorage(int teamId, int unitDefId, int resourceId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isSquareResourceExtractor(int teamId, int unitDefId, int resourceId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isResourceMaker(int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getAutoHeal(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getIdleAutoHeal(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getIdleTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getPower(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getHealth(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getCategory(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnRate(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isTurnInPlace(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnInPlaceDistance(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit(int teamId, int unitDefId);
//EXPORT(int              ) skirmishAiCallback_UnitDef_getMoveType(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isUpright(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isCollide(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getControlRadius(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getLosRadius(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getAirLosRadius(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getLosHeight(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getRadarRadius(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getSonarRadius(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getJammerRadius(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getSonarJamRadius(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getSeismicRadius(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getSeismicSignature(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isStealth(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isSonarStealth(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isBuildRange3D(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildDistance(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getReclaimSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getRepairSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxRepairSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getResurrectSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getCaptureSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTerraformSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMass(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isPushResistant(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isStrafeToAttack(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinCollisionSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getSlideTolerance(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxSlope(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxHeightDif(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinWaterDepth(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getWaterline(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxWaterDepth(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getArmoredMultiple(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getArmorType(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_FlankingBonus_getMode(int teamId, int unitDefId);
EXPORT(void             ) skirmishAiCallback_UnitDef_FlankingBonus_getDir(int teamId, int unitDefId, float* return_posF3_out);
EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMax(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMin(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_CollisionVolume_getType(int teamId, int unitDefId);
EXPORT(void             ) skirmishAiCallback_UnitDef_CollisionVolume_getScales(int teamId, int unitDefId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_UnitDef_CollisionVolume_getOffsets(int teamId, int unitDefId, float* return_posF3_out);
EXPORT(int              ) skirmishAiCallback_UnitDef_CollisionVolume_getTest(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxWeaponRange(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getType(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getTooltip(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getWreckName(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getDeathExplosion(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getSelfDExplosion(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getTedClassString(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getCategoryString(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToSelfD(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getSelfDCountdown(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToSubmerge(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToFly(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToMove(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToHover(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isFloater(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isBuilder(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isActivateWhenBuilt(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isOnOffable(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isFullHealthFactory(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isReclaimable(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isCapturable(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToRestore(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToRepair(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToSelfRepair(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToReclaim(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToAttack(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToPatrol(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToFight(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToGuard(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToAssist(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAssistable(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToRepeat(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToFireControl(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFireState(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getMoveState(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getWingDrag(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getWingAngle(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getDrag(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getFrontToSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getSpeedToFront(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMyGravity(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxBank(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxPitch(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnRadius(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getWantedHeight(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getVerticalSpeed(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToCrash(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isHoverAttack(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAirStrafe(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getDlHoverFactor(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxAcceleration(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxDeceleration(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxAileron(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxElevator(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxRudder(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getYardMap(int teamId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax);
EXPORT(int              ) skirmishAiCallback_UnitDef_getXSize(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getZSize(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildAngle(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getLoadingRadius(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getUnloadSpread(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportCapacity(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportSize(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getMinTransportSize(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAirBase(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isFirePlatform(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTransportMass(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinTransportMass(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isHoldSteady(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isReleaseHeld(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isNotTransportable(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isTransportByEnemy(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportUnloadMethod(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getFallSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getUnitFallSpeed(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToCloak(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isStartCloaked(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getCloakCost(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getCloakCostMoving(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getDecloakDistance(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isDecloakSpherical(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isDecloakOnFire(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToKamikaze(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getKamikazeDist(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isTargetingFacility(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToDGun(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isNeedGeo(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isFeature(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isHideDamage(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isCommander(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isShowPlayerName(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToResurrect(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToCapture(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getHighTrajectoryType(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getNoChaseCategory(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isLeaveTracks(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackWidth(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackOffset(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackStrength(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackStretch(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getTrackType(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToDropFlare(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getFlareReloadTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getFlareEfficiency(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getFlareDelay(int teamId, int unitDefId);
EXPORT(void             ) skirmishAiCallback_UnitDef_getFlareDropVector(int teamId, int unitDefId, float* return_posF3_out);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareTime(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareSalvoSize(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareSalvoDelay(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToLoopbackAttack(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isLevelGround(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isUseBuildingGroundDecal(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalType(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalSizeX(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalSizeY(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxFuel(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getRefuelTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinAirBasePower(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getMaxThisUnit(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getDecoyDef(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isDontLand(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getShieldDef(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getStockpileDef(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildOptions(int teamId, int unitDefId, int* unitDefIds, int unitDefIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_UnitDef_getCustomParams(int teamId, int unitDefId, const char** keys, const char** values);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isMoveDataAvailable(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getMoveType(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getMoveFamily(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getSize(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getDepth(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxSlope(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getSlopeMod(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getDepthMod(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getPathType(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getCrushStrength(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxSpeed(int teamId, int unitDefId);
EXPORT(short            ) skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxBreaking(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_MoveData_getFollowGround(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_MoveData_isSubMarine(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_MoveData_getName(int teamId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getWeaponMounts(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_WeaponMount_getName(int teamId, int unitDefId, int weaponMountId);
EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getWeaponDef(int teamId, int unitDefId, int weaponMountId);
EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo(int teamId, int unitDefId, int weaponMountId);
EXPORT(void             ) skirmishAiCallback_UnitDef_WeaponMount_getMainDir(int teamId, int unitDefId, int weaponMountId, float* return_posF3_out);
EXPORT(float            ) skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif(int teamId, int unitDefId, int weaponMountId);
EXPORT(float            ) skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage(int teamId, int unitDefId, int weaponMountId);
EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory(int teamId, int unitDefId, int weaponMountId);
EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory(int teamId, int unitDefId, int weaponMountId);
// END OBJECT UnitDef



// BEGINN OBJECT Unit
EXPORT(int              ) skirmishAiCallback_Unit_getLimit(int teamId);
EXPORT(int              ) skirmishAiCallback_getEnemyUnits(int teamId, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getEnemyUnitsIn(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getEnemyUnitsInRadarAndLos(int teamId, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getFriendlyUnits(int teamId, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getFriendlyUnitsIn(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getNeutralUnits(int teamId, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getNeutralUnitsIn(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getTeamUnits(int teamId, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getSelectedUnits(int teamId, int* unitIds, int unitIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_Unit_getDef(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getModParams(int teamId, int unitId);
EXPORT(const char*      ) skirmishAiCallback_Unit_ModParam_getName(int teamId, int unitId, int modParamId);
EXPORT(float            ) skirmishAiCallback_Unit_ModParam_getValue(int teamId, int unitId, int modParamId);
EXPORT(int              ) skirmishAiCallback_Unit_getTeam(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getAllyTeam(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getLineage(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getAiHint(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getStockpile(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getStockpileQueued(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getCurrentFuel(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getMaxSpeed(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getMaxRange(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getMaxHealth(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getExperience(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getGroup(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getCurrentCommands(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getType(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getId(int teamId, int unitId, int commandId);
EXPORT(short            ) skirmishAiCallback_Unit_CurrentCommand_getOptions(int teamId, int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getTag(int teamId, int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int teamId, int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getParams(int teamId, int unitId, int commandId, float* params, int params_sizeMax);
EXPORT(int              ) skirmishAiCallback_Unit_getSupportedCommands(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_getId(int teamId, int unitId, int commandId);
EXPORT(const char*      ) skirmishAiCallback_Unit_SupportedCommand_getName(int teamId, int unitId, int commandId);
EXPORT(const char*      ) skirmishAiCallback_Unit_SupportedCommand_getToolTip(int teamId, int unitId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Unit_SupportedCommand_isShowUnique(int teamId, int unitId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Unit_SupportedCommand_isDisabled(int teamId, int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_getParams(int teamId, int unitId, int commandId, const char** params, int params_sizeMax);
EXPORT(float            ) skirmishAiCallback_Unit_getHealth(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getSpeed(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getPower(int teamId, int unitId);
//EXPORT(int              ) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getResourceUse(int teamId, int unitId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Unit_getResourceMake(int teamId, int unitId, int resourceId);
EXPORT(void             ) skirmishAiCallback_Unit_getPos(int teamId, int unitId, float* return_posF3_out);
EXPORT(bool             ) skirmishAiCallback_Unit_isActivated(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isBeingBuilt(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isCloaked(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isParalyzed(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isNeutral(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getBuildingFacing(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getLastUserOrderFrame(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
EXPORT(int              ) skirmishAiCallback_getGroups(int teamId, int* groupIds, int groupIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_Group_getSupportedCommands(int teamId, int groupId);
EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_getId(int teamId, int groupId, int commandId);
EXPORT(const char*      ) skirmishAiCallback_Group_SupportedCommand_getName(int teamId, int groupId, int commandId);
EXPORT(const char*      ) skirmishAiCallback_Group_SupportedCommand_getToolTip(int teamId, int groupId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Group_SupportedCommand_isShowUnique(int teamId, int groupId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Group_SupportedCommand_isDisabled(int teamId, int groupId, int commandId);
EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_getParams(int teamId, int groupId, int commandId, const char** params, int params_sizeMax);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getId(int teamId, int groupId);
EXPORT(short            ) skirmishAiCallback_Group_OrderPreview_getOptions(int teamId, int groupId);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getTag(int teamId, int groupId);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getTimeOut(int teamId, int groupId);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getParams(int teamId, int groupId, float* params, int params_sizeMax);
EXPORT(bool             ) skirmishAiCallback_Group_isSelected(int teamId, int groupId);
// END OBJECT Group



// BEGINN OBJECT Mod
EXPORT(const char*      ) skirmishAiCallback_Mod_getFileName(int teamId);

EXPORT(const char*      ) skirmishAiCallback_Mod_getHumanName(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Mod_getShortName(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Mod_getVersion(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Mod_getMutator(int teamId);
EXPORT(const char*      ) skirmishAiCallback_Mod_getDescription(int teamId);

EXPORT(bool             ) skirmishAiCallback_Mod_getAllowTeamColors(int teamId);

EXPORT(bool             ) skirmishAiCallback_Mod_getConstructionDecay(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getConstructionDecayTime(int teamId);
EXPORT(float            ) skirmishAiCallback_Mod_getConstructionDecaySpeed(int teamId);

EXPORT(int              ) skirmishAiCallback_Mod_getMultiReclaim(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getReclaimMethod(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getReclaimUnitMethod(int teamId);
EXPORT(float            ) skirmishAiCallback_Mod_getReclaimUnitEnergyCostFactor(int teamId);
EXPORT(float            ) skirmishAiCallback_Mod_getReclaimUnitEfficiency(int teamId);
EXPORT(float            ) skirmishAiCallback_Mod_getReclaimFeatureEnergyCostFactor(int teamId);
EXPORT(bool             ) skirmishAiCallback_Mod_getReclaimAllowEnemies(int teamId);
EXPORT(bool             ) skirmishAiCallback_Mod_getReclaimAllowAllies(int teamId);

EXPORT(float            ) skirmishAiCallback_Mod_getRepairEnergyCostFactor(int teamId);

EXPORT(float            ) skirmishAiCallback_Mod_getResurrectEnergyCostFactor(int teamId);

EXPORT(float            ) skirmishAiCallback_Mod_getCaptureEnergyCostFactor(int teamId);

EXPORT(int              ) skirmishAiCallback_Mod_getTransportGround(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getTransportHover(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getTransportShip(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getTransportAir(int teamId);

EXPORT(int              ) skirmishAiCallback_Mod_getFireAtKilled(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getFireAtCrashing(int teamId);

EXPORT(int              ) skirmishAiCallback_Mod_getFlankingBonusModeDefault(int teamId);

EXPORT(int              ) skirmishAiCallback_Mod_getLosMipLevel(int teamId);
EXPORT(int              ) skirmishAiCallback_Mod_getAirMipLevel(int teamId);
EXPORT(float            ) skirmishAiCallback_Mod_getLosMul(int teamId);
EXPORT(float            ) skirmishAiCallback_Mod_getAirLosMul(int teamId);
EXPORT(bool             ) skirmishAiCallback_Mod_getRequireSonarUnderWater(int teamId);
// END OBJECT Mod



// BEGINN OBJECT Map
EXPORT(int              ) skirmishAiCallback_Map_getChecksum(int teamId);
EXPORT(void             ) skirmishAiCallback_Map_getStartPos(int teamId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_Map_getMousePos(int teamId, float* return_posF3_out);
EXPORT(bool             ) skirmishAiCallback_Map_isPosInCamera(int teamId, float* pos_posF3, float radius);
EXPORT(int              ) skirmishAiCallback_Map_getWidth(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_getHeight(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_getHeightMap(int teamId, float* heights, int heights_sizeMax);
EXPORT(int              ) skirmishAiCallback_Map_getCornersHeightMap(int teamId, float* cornerHeights, int cornerHeights_sizeMax);
EXPORT(float            ) skirmishAiCallback_Map_getMinHeight(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getMaxHeight(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_getSlopeMap(int teamId, float* slopes, int slopes_sizeMax);
EXPORT(int              ) skirmishAiCallback_Map_getLosMap(int teamId, int* losValues, int losValues_sizeMax);
EXPORT(int              ) skirmishAiCallback_Map_getRadarMap(int teamId, int* radarValues, int radarValues_sizeMax);
EXPORT(int              ) skirmishAiCallback_Map_getJammerMap(int teamId, int* jammerValues, int jammerValues_sizeMax);
EXPORT(int              ) skirmishAiCallback_Map_getResourceMapRaw(int teamId, int resourceId, short* resources, int resources_sizeMax);
EXPORT(int              ) skirmishAiCallback_Map_getResourceMapSpotsPositions(int teamId, int resourceId, float* spots_AposF3, int spots_AposF3_sizeMax);
EXPORT(float            ) skirmishAiCallback_Map_initResourceMapSpotsNearest(int teamId, int resourceId, float* pos_posF3, float* return_posF3_out);
EXPORT(const char*      ) skirmishAiCallback_Map_getName(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getElevationAt(int teamId, float x, float z);
EXPORT(float            ) skirmishAiCallback_Map_getMaxResource(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Map_getExtractorRadius(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Map_getMinWind(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getMaxWind(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getTidalStrength(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getGravity(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_getPoints(int teamId, bool includeAllies);
EXPORT(void             ) skirmishAiCallback_Map_Point_getPosition(int teamId, int pointId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_Map_Point_getColor(int teamId, int pointId, short* return_colorS3_out);
EXPORT(const char*      ) skirmishAiCallback_Map_Point_getLabel(int teamId, int pointId);
EXPORT(int              ) skirmishAiCallback_Map_getLines(int teamId, bool includeAllies);
EXPORT(void             ) skirmishAiCallback_Map_Line_getFirstPosition(int teamId, int lineId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_Map_Line_getSecondPosition(int teamId, int lineId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_Map_Line_getColor(int teamId, int lineId, short* return_colorS3_out);
EXPORT(bool             ) skirmishAiCallback_Map_isPossibleToBuildAt(int teamId, int unitDefId, float* pos_posF3, int facing);
EXPORT(void             ) skirmishAiCallback_Map_findClosestBuildSite(int teamId, int unitDefId, float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out);
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
EXPORT(int              ) skirmishAiCallback_getFeatureDefs(int teamId, int* featureDefIds, int featureDefIds_sizeMax);
//int) skirmishAiCallback_FeatureDef_getId(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getName(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getDescription(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getFileName(int teamId, int featureDefId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getContainedResource(int teamId, int featureDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getMaxHealth(int teamId, int featureDefId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getReclaimTime(int teamId, int featureDefId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getMass(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_CollisionVolume_getType(int teamId, int featureDefId);
EXPORT(void             ) skirmishAiCallback_FeatureDef_CollisionVolume_getScales(int teamId, int featureDefId, float* return_posF3_out);
EXPORT(void             ) skirmishAiCallback_FeatureDef_CollisionVolume_getOffsets(int teamId, int featureDefId, float* return_posF3_out);
EXPORT(int              ) skirmishAiCallback_FeatureDef_CollisionVolume_getTest(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isUpright(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getDrawType(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getModelName(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getResurrectable(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getSmokeTime(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isDestructable(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isReclaimable(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isBlocking(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isBurnable(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isFloating(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isNoSelect(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isGeoThermal(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getDeathFeature(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getXSize(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getZSize(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getCustomParams(int teamId, int featureDefId, const char** keys, const char** values);
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
EXPORT(int              ) skirmishAiCallback_getFeatures(int teamId, int* featureIds, int featureIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_getFeaturesIn(int teamId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax);
EXPORT(int              ) skirmishAiCallback_Feature_getDef(int teamId, int featureId);
EXPORT(float            ) skirmishAiCallback_Feature_getHealth(int teamId, int featureId);
EXPORT(float            ) skirmishAiCallback_Feature_getReclaimLeft(int teamId, int featureId);
EXPORT(void             ) skirmishAiCallback_Feature_getPosition(int teamId, int featureId, float* return_posF3_out);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
EXPORT(int              ) skirmishAiCallback_getWeaponDefs(int teamId);
EXPORT(int              ) skirmishAiCallback_getWeaponDefByName(int teamId, const char* weaponDefName);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getName(int teamId, int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getType(int teamId, int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getDescription(int teamId, int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getFileName(int teamId, int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getCegTag(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getRange(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getHeightMod(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getAccuracy(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSprayAngle(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMovingAccuracy(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTargetMoveError(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getLeadLimit(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getLeadBonus(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getPredictBoost(int teamId, int weaponDefId);
// Deprecate the following function, if no longer needed by legacy Cpp AIs
EXPORT(int              ) skirmishAiCallback_WeaponDef_getNumDamageTypes(int teamId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getImpulseFactor(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getImpulseBoost(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getCraterMult(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getCraterBoost(int teamId, int weaponDefId);
//EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getType(int teamId, int weaponDefId, int typeId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_getTypes(int teamId, int weaponDefId, float* types, int types_sizeMax);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getAreaOfEffect(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoSelfDamage(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getFireStarter(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getEdgeEffectiveness(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSize(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSizeGrowth(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCollisionSize(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getSalvoSize(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSalvoDelay(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getReload(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getBeamTime(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isBeamBurst(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isWaterBounce(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isGroundBounce(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getBounceRebound(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getBounceSlip(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getNumBounce(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMaxAngle(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getRestTime(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getUpTime(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getFlightTime(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCost(int teamId, int weaponDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSupplyCost(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getProjectilesPerShot(int teamId, int weaponDefId);
//EXPORT(int              ) skirmishAiCallback_WeaponDef_getTdfId(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isTurret(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isOnlyForward(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isFixedLauncher(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isWaterWeapon(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isFireSubmersed(int teamId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSubMissile(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isTracks(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isDropped(int teamId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isParalyzer(int teamId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isImpactOnly(int teamId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoAutoTarget(int teamId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isManualFire(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getInterceptor(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getTargetable(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isStockpileable(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCoverageRange(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getStockpileTime(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getIntensity(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getThickness(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getLaserFlareSize(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCoreThickness(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDuration(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getLodDistance(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getFalloffRate(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getGraphicsType(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSoundTrigger(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSelfExplode(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isGravityAffected(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getHighTrajectory(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMyGravity(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoExplode(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getStartVelocity(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getWeaponAcceleration(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTurnRate(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMaxVelocity(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getProjectileSpeed(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getExplosionSpeed(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getWobble(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDance(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTrajectoryHeight(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isLargeBeamLaser(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isShieldRepulser(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSmartShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isExteriorShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isVisibleShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isVisibleShieldRepulse(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getResourceUse(int teamId, int weaponDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getRadius(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getForce(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getMaxSpeed(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPower(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPowerRegen(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPowerRegenResource(int teamId, int weaponDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int teamId, int weaponDefId);
EXPORT(void             ) skirmishAiCallback_WeaponDef_Shield_getGoodColor(int teamId, int weaponDefId, short* return_colorS3_out);
EXPORT(void             ) skirmishAiCallback_WeaponDef_Shield_getBadColor(int teamId, int weaponDefId, short* return_colorS3_out);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getAlpha(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Shield_getInterceptType(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getInterceptedByShieldType(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidFriendly(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidFeature(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidNeutral(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTargetBorder(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCylinderTargetting(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMinIntensity(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getHeightBoostFactor(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getProximityPriority(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getCollisionFlags(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSweepFire(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAbleToAttackGround(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCameraShake(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageExp(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageMin(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageRange(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isDynDamageInverted(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getCustomParams(int teamId, int weaponDefId, const char** keys, const char** values);
// END OBJECT WeaponDef

#if	defined(__cplusplus)
} // extern "C"
#endif

#if defined __cplusplus && !defined BUILDING_AI
class IGlobalAICallback;

// for engine internal use only
SSkirmishAICallback* skirmishAiCallback_getInstanceFor(int teamId, IGlobalAICallback* aiGlobalCallback);
void skirmishAiCallback_release(int teamId);
#endif // defined __cplusplus && !defined BUILDING_AI

#endif // _SSKIRMISHAICALLBACKIMPL_H
