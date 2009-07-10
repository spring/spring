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

EXPORT(int              ) skirmishAiCallback_Engine_handleCommand(int teamId, int toId, int commandId,
		int commandTopic, void* commandData);

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
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Gui_Camera_getDirection(int teamId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Gui_Camera_getPosition(int teamId);
// END Visualization related callback functions


// BEGINN OBJECT Cheats
EXPORT(bool             ) skirmishAiCallback_Cheats_isEnabled(int teamId);
EXPORT(bool             ) skirmishAiCallback_Cheats_setEnabled(int teamId, bool enable);
EXPORT(bool             ) skirmishAiCallback_Cheats_setEventsEnabled(int teamId, bool enabled);
EXPORT(bool             ) skirmishAiCallback_Cheats_isOnlyPassive(int teamId);
// END OBJECT Cheats


// BEGINN OBJECT Resource
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE0Resource(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1FETCH3ResourceByName0Resource(int teamId,
		const char* resourceName);
EXPORT(const char*      ) skirmishAiCallback_Resource_getName(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Resource_getOptimum(int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_0REF1Resource2resourceId0getCurrent(
		int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_0REF1Resource2resourceId0getIncome(int teamId,
		int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_0REF1Resource2resourceId0getUsage(int teamId,
		int resourceId);
EXPORT(float            ) skirmishAiCallback_Economy_0REF1Resource2resourceId0getStorage(
		int teamId, int resourceId);
// END OBJECT Resource


// BEGINN OBJECT File
EXPORT(int              ) skirmishAiCallback_File_getSize(int teamId, const char* fileName);
EXPORT(bool             ) skirmishAiCallback_File_getContent(int teamId, const char* fileName,
		void* buffer, int bufferLen);
// EXPORT(bool             ) skirmishAiCallback_File_locateForReading(int teamId, char* fileName, int fileName_sizeMax);
// EXPORT(bool             ) skirmishAiCallback_File_locateForWriting(int teamId, char* fileName, int fileName_sizeMax);
// END OBJECT File


// BEGINN OBJECT UnitDef
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE0UnitDef(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS0UnitDef(int teamId, int unitDefIds[],
		int unitDefIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1FETCH3UnitDefByName0UnitDef(int teamId,
		const char* unitName);
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
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getUpkeep(int teamId,
		int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getResourceMake(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getMakesResource(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getCost(int teamId,
		int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getExtractsResource(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getStorage(
		int teamId, int unitDefId, int resourceId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor(
		int teamId, int unitDefId, int resourceId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0isResourceMaker(
		int teamId, int unitDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getAutoHeal(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getIdleAutoHeal(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getIdleTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getPower(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getHealth(int teamId, int unitDefId);
EXPORT(unsigned int     ) skirmishAiCallback_UnitDef_getCategory(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getSpeed(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnRate(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isTurnInPlace(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnInPlaceDistance(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getMoveType(int teamId, int unitDefId);
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
EXPORT(float            ) skirmishAiCallback_UnitDef_getSeismicSignature(int teamId,
		int unitDefId);
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
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinCollisionSpeed(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getSlideTolerance(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxSlope(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxHeightDif(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinWaterDepth(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getWaterline(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxWaterDepth(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getArmoredMultiple(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getArmorType(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_FlankingBonus_getMode(int teamId,
		int unitDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_UnitDef_FlankingBonus_getDir(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMax(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMin(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd(int teamId,
		int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_CollisionVolume_getType(int teamId,
		int unitDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_UnitDef_CollisionVolume_getScales(
		int teamId, int unitDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_UnitDef_CollisionVolume_getOffsets(
		int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_CollisionVolume_getTest(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxWeaponRange(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getType(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getTooltip(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getWreckName(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getDeathExplosion(int teamId,
		int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getSelfDExplosion(int teamId,
		int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getTedClassString(int teamId,
		int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_getCategoryString(int teamId,
		int unitDefId);
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
EXPORT(bool             ) skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff(int teamId,
		int unitDefId);
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
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinTransportMass(int teamId,
		int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isHoldSteady(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isReleaseHeld(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isNotTransportable(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isTransportByEnemy(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportUnloadMethod(int teamId,
		int unitDefId);
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
EXPORT(unsigned int     ) skirmishAiCallback_UnitDef_getNoChaseCategory(int teamId,
		int unitDefId);
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
EXPORT(struct SAIFloat3 ) skirmishAiCallback_UnitDef_getFlareDropVector(int teamId,
		int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareTime(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareSalvoSize(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareSalvoDelay(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToLoopbackAttack(int teamId,
		int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isLevelGround(int teamId, int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isUseBuildingGroundDecal(int teamId,
		int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalType(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalSizeX(int teamId,
		int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalSizeY(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed(int teamId,
		int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxFuel(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getRefuelTime(int teamId, int unitDefId);
EXPORT(float            ) skirmishAiCallback_UnitDef_getMinAirBasePower(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_getMaxThisUnit(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef(int teamId,
		int unitDefId);
EXPORT(bool             ) skirmishAiCallback_UnitDef_isDontLand(int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef(int teamId,
		int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef(
		int teamId, int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions(int teamId,
		int unitDefId);
EXPORT(int              ) skirmishAiCallback_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions(int teamId,
		int unitDefId, int unitDefIds[], int unitDefIds_max);
EXPORT(int              ) skirmishAiCallback_UnitDef_0MAP1SIZE0getCustomParams(int teamId,
		int unitDefId);
EXPORT(void) skirmishAiCallback_UnitDef_0MAP1KEYS0getCustomParams(int teamId,
		int unitDefId, const char* keys[]);
EXPORT(void) skirmishAiCallback_UnitDef_0MAP1VALS0getCustomParams(int teamId,
		int unitDefId, const char* values[]);



EXPORT(bool             ) skirmishAiCallback_UnitDef_0AVAILABLE0MoveData(int teamId, int unitDefId);
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



EXPORT(int              ) skirmishAiCallback_UnitDef_0MULTI1SIZE0WeaponMount(int teamId, int unitDefId);
EXPORT(const char*      ) skirmishAiCallback_UnitDef_WeaponMount_getName(int teamId, int unitDefId, int weaponMountId);
EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef(
		int teamId, int unitDefId, int weaponMountId);
EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo(int teamId,
		int unitDefId, int weaponMountId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_UnitDef_WeaponMount_getMainDir(int teamId,
		int unitDefId, int weaponMountId);
EXPORT(float            ) skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif(int teamId,
		int unitDefId, int weaponMountId);
EXPORT(float            ) skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage(int teamId,
		int unitDefId, int weaponMountId);
EXPORT(unsigned int     ) skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory(
		int teamId, int unitDefId, int weaponMountId);
EXPORT(unsigned int     ) skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory(
		int teamId, int unitDefId, int weaponMountId);
// END OBJECT UnitDef



// BEGINN OBJECT Unit
EXPORT(int              ) skirmishAiCallback_Unit_0STATIC0getLimit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3EnemyUnits0Unit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3EnemyUnits0Unit(int teamId, int unitIds[],
		int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3EnemyUnitsIn0Unit(int teamId,
		struct SAIFloat3 pos, float radius);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3EnemyUnitsIn0Unit(int teamId,
		struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit(int teamId,
		int unitIds[], int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3FriendlyUnits0Unit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3FriendlyUnits0Unit(int teamId,
		int unitIds[], int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3FriendlyUnitsIn0Unit(int teamId,
		struct SAIFloat3 pos, float radius);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3FriendlyUnitsIn0Unit(int teamId,
		struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3NeutralUnits0Unit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3NeutralUnits0Unit(int teamId, int unitIds[],
		int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3NeutralUnitsIn0Unit(int teamId,
		struct SAIFloat3 pos, float radius);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3NeutralUnitsIn0Unit(int teamId,
		struct SAIFloat3 pos, float radius, int unitIds[], int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3TeamUnits0Unit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3TeamUnits0Unit(int teamId, int unitIds[],
		int unitIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3SelectedUnits0Unit(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3SelectedUnits0Unit(int teamId,
		int unitIds[], int unitIds_max);
EXPORT(int              ) skirmishAiCallback_Unit_0SINGLE1FETCH2UnitDef0getDef(int teamId,
		int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_0MULTI1SIZE0ModParam(int teamId, int unitId);
EXPORT(const char*      ) skirmishAiCallback_Unit_ModParam_getName(int teamId, int unitId,
		int modParamId);
EXPORT(float            ) skirmishAiCallback_Unit_ModParam_getValue(int teamId, int unitId,
		int modParamId);
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
EXPORT(int              ) skirmishAiCallback_Unit_0MULTI1SIZE1Command0CurrentCommand(int teamId,
		int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_0STATIC0getType(int teamId,
		int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getId(int teamId, int unitId,
		int commandId);
EXPORT(unsigned char) skirmishAiCallback_Unit_CurrentCommand_getOptions(int teamId,
		int unitId, int commandId);
EXPORT(unsigned int     ) skirmishAiCallback_Unit_CurrentCommand_getTag(int teamId,
		int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int teamId, int unitId,
		int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_0ARRAY1SIZE0getParams(int teamId,
		int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_0ARRAY1VALS0getParams(int teamId,
		int unitId, int commandId, float params[], int params_max);
EXPORT(int              ) skirmishAiCallback_Unit_0MULTI1SIZE0SupportedCommand(int teamId,
		int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_getId(int teamId, int unitId,
		int commandId);
EXPORT(const char*      ) skirmishAiCallback_Unit_SupportedCommand_getName(int teamId,
		int unitId, int commandId);
EXPORT(const char*      ) skirmishAiCallback_Unit_SupportedCommand_getToolTip(int teamId,
		int unitId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Unit_SupportedCommand_isShowUnique(int teamId,
		int unitId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Unit_SupportedCommand_isDisabled(int teamId,
		int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_0ARRAY1SIZE0getParams(int teamId,
		int unitId, int commandId);
EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_0ARRAY1VALS0getParams(int teamId,
		int unitId, int commandId, const char* params[], int params_max);
EXPORT(float            ) skirmishAiCallback_Unit_getHealth(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getSpeed(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_getPower(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int teamId, int unitId);
EXPORT(float            ) skirmishAiCallback_Unit_0REF1Resource2resourceId0getResourceUse(
		int teamId, int unitId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Unit_0REF1Resource2resourceId0getResourceMake(
		int teamId, int unitId, int resourceId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Unit_getPos(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isActivated(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isBeingBuilt(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isCloaked(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isParalyzed(int teamId, int unitId);
EXPORT(bool             ) skirmishAiCallback_Unit_isNeutral(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getBuildingFacing(int teamId, int unitId);
EXPORT(int              ) skirmishAiCallback_Unit_getLastUserOrderFrame(int teamId, int unitId);
// END OBJECT Unit


// BEGINN OBJECT Group
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE0Group(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS0Group(int teamId, int groupIds[],
		int groupIds_max);
EXPORT(int              ) skirmishAiCallback_Group_0MULTI1SIZE0SupportedCommand(int teamId,
		int groupId);
EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_getId(int teamId, int groupId,
		int commandId);
EXPORT(const char*      ) skirmishAiCallback_Group_SupportedCommand_getName(int teamId,
		int groupId, int commandId);
EXPORT(const char*      ) skirmishAiCallback_Group_SupportedCommand_getToolTip(int teamId,
		int groupId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Group_SupportedCommand_isShowUnique(int teamId,
		int groupId, int commandId);
EXPORT(bool             ) skirmishAiCallback_Group_SupportedCommand_isDisabled(int teamId,
		int groupId, int commandId);
EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_0ARRAY1SIZE0getParams(int teamId,
		int groupId, int commandId);
EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_0ARRAY1VALS0getParams(int teamId,
		int groupId, int commandId, const char* params[], int params_max);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getId(int teamId, int groupId);
EXPORT(unsigned char) skirmishAiCallback_Group_OrderPreview_getOptions(int teamId,
		int groupId);
EXPORT(unsigned int     ) skirmishAiCallback_Group_OrderPreview_getTag(int teamId,
		int groupId);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getTimeOut(int teamId, int groupId);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_0ARRAY1SIZE0getParams(int teamId,
		int groupId);
EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_0ARRAY1VALS0getParams(int teamId,
		int groupId, float params[], int params_max);
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
EXPORT(unsigned int     ) skirmishAiCallback_Map_getChecksum(int teamId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_getStartPos(int teamId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_getMousePos(int teamId);
EXPORT(bool             ) skirmishAiCallback_Map_isPosInCamera(int teamId, struct SAIFloat3 pos,
		float radius);
EXPORT(int              ) skirmishAiCallback_Map_getWidth(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_getHeight(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0getHeightMap(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0getHeightMap(int teamId,
		float heights[], int heights_max);
EXPORT(float            ) skirmishAiCallback_Map_getMinHeight(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getMaxHeight(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0getSlopeMap(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0getSlopeMap(int teamId, float slopes[],
		int slopes_max);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0getLosMap(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0getLosMap(int teamId,
		unsigned short losValues[], int losValues_max);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0getRadarMap(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0getRadarMap(int teamId,
		unsigned short radarValues[], int radarValues_max);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0getJammerMap(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0getJammerMap(int teamId,
		unsigned short jammerValues[], int jammerValues_max);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw(
		int teamId, int resourceId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapRaw(
		int teamId, int resourceId, unsigned char resources[], int resources_max);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapSpotsPositions(
		int teamId, int resourceId);
EXPORT(int              ) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapSpotsPositions(
		int teamId, int resourceId, SAIFloat3 spots[], int spots_max);
EXPORT(float            ) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsAverageIncome(
		int teamId, int resourceId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsNearest(
		int teamId, int resourceId, struct SAIFloat3 pos);
EXPORT(const char*      ) skirmishAiCallback_Map_getName(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getElevationAt(int teamId, float x, float z);
EXPORT(float            ) skirmishAiCallback_Map_0REF1Resource2resourceId0getMaxResource(
		int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Map_0REF1Resource2resourceId0getExtractorRadius(
		int teamId, int resourceId);
EXPORT(float            ) skirmishAiCallback_Map_getMinWind(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getMaxWind(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getTidalStrength(int teamId);
EXPORT(float            ) skirmishAiCallback_Map_getGravity(int teamId);
EXPORT(int              ) skirmishAiCallback_Map_0MULTI1SIZE0Point(int teamId,
		bool includeAllies);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_Point_getPosition(int teamId,
		int pointId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_Point_getColor(int teamId,
		int pointId);
EXPORT(const char*      ) skirmishAiCallback_Map_Point_getLabel(int teamId,
		int pointId);
EXPORT(int              ) skirmishAiCallback_Map_0MULTI1SIZE0Line(int teamId,
		bool includeAllies);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_Line_getFirstPosition(
		int teamId, int lineId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_Line_getSecondPosition(
		int teamId, int lineId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_Line_getColor(int teamId,
		int lineId);
EXPORT(bool             ) skirmishAiCallback_Map_0REF1UnitDef2unitDefId0isPossibleToBuildAt(int teamId, int unitDefId,
		struct SAIFloat3 pos, int facing);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Map_0REF1UnitDef2unitDefId0findClosestBuildSite(int teamId,
		int unitDefId, struct SAIFloat3 pos, float searchRadius, int minDist,
		int facing);
// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE0FeatureDef(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS0FeatureDef(int teamId, int featureDefIds[],
		int featureDefIds_max);
//int) skirmishAiCallback_FeatureDef_getId(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getName(int teamId,
		int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getDescription(int teamId,
		int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getFileName(int teamId,
		int featureDefId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_0REF1Resource2resourceId0getContainedResource(
		int teamId, int featureDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getMaxHealth(int teamId, int featureDefId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getReclaimTime(int teamId,
		int featureDefId);
EXPORT(float            ) skirmishAiCallback_FeatureDef_getMass(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_CollisionVolume_getType(int teamId,
		int featureDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_FeatureDef_CollisionVolume_getScales(
		int teamId, int featureDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_FeatureDef_CollisionVolume_getOffsets(
		int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_CollisionVolume_getTest(int teamId,
		int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isUpright(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getDrawType(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getModelName(int teamId,
		int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getResurrectable(int teamId,
		int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getSmokeTime(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isDestructable(int teamId,
		int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isReclaimable(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isBlocking(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isBurnable(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isFloating(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isNoSelect(int teamId, int featureDefId);
EXPORT(bool             ) skirmishAiCallback_FeatureDef_isGeoThermal(int teamId, int featureDefId);
EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getDeathFeature(int teamId,
		int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getXSize(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_getZSize(int teamId, int featureDefId);
EXPORT(int              ) skirmishAiCallback_FeatureDef_0MAP1SIZE0getCustomParams(int teamId,
		int featureDefId);
EXPORT(void) skirmishAiCallback_FeatureDef_0MAP1KEYS0getCustomParams(int teamId,
		int featureDefId, const char* keys[]);
EXPORT(void) skirmishAiCallback_FeatureDef_0MAP1VALS0getCustomParams(int teamId,
		int featureDefId, const char* values[]);
// END OBJECT FeatureDef


// BEGINN OBJECT Feature
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE0Feature(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS0Feature(int teamId, int featureIds[],
		int featureIds_max);
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE3FeaturesIn0Feature(int teamId,
		struct SAIFloat3 pos, float radius);
EXPORT(int              ) skirmishAiCallback_0MULTI1VALS3FeaturesIn0Feature(int teamId,
		struct SAIFloat3 pos, float radius, int featureIds[],
		int featureIds_max);
EXPORT(int              ) skirmishAiCallback_Feature_0SINGLE1FETCH2FeatureDef0getDef(int teamId,
		int featureId);
EXPORT(float            ) skirmishAiCallback_Feature_getHealth(int teamId, int featureId);
EXPORT(float            ) skirmishAiCallback_Feature_getReclaimLeft(int teamId, int featureId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_Feature_getPosition(int teamId,
		int featureId);
// END OBJECT Feature



// BEGINN OBJECT WeaponDef
EXPORT(int              ) skirmishAiCallback_0MULTI1SIZE0WeaponDef(int teamId);
EXPORT(int              ) skirmishAiCallback_0MULTI1FETCH3WeaponDefByName0WeaponDef(int teamId,
		const char* weaponDefName);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getName(int teamId, int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getType(int teamId, int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getDescription(int teamId,
		int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getFileName(int teamId,
		int weaponDefId);
EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getCegTag(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getRange(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getHeightMod(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getAccuracy(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSprayAngle(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMovingAccuracy(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTargetMoveError(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getLeadLimit(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getLeadBonus(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getPredictBoost(int teamId,
		int weaponDefId);
// Deprecate the following function, if no longer needed by legacy Cpp AIs
EXPORT(int              ) skirmishAiCallback_WeaponDef_0STATIC0getNumDamageTypes(int teamId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getImpulseFactor(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getImpulseBoost(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getCraterMult(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getCraterBoost(int teamId,
		int weaponDefId);
//EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getType(int teamId, int weaponDefId, int typeId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_0ARRAY1SIZE0getTypes(int teamId,
		int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_0ARRAY1VALS0getTypes(int teamId,
		int weaponDefId, float types[], int types_max);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getAreaOfEffect(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoSelfDamage(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getFireStarter(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getEdgeEffectiveness(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSize(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSizeGrowth(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCollisionSize(int teamId,
		int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getSalvoSize(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSalvoDelay(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getReload(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getBeamTime(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isBeamBurst(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isWaterBounce(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isGroundBounce(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getBounceRebound(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getBounceSlip(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getNumBounce(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMaxAngle(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getRestTime(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getUpTime(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getFlightTime(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_0REF1Resource2resourceId0getCost(int teamId,
		int weaponDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getSupplyCost(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getProjectilesPerShot(int teamId,
		int weaponDefId);
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
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCoverageRange(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getStockpileTime(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getIntensity(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getThickness(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getLaserFlareSize(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCoreThickness(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDuration(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getLodDistance(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getFalloffRate(int teamId, int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getGraphicsType(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSoundTrigger(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSelfExplode(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isGravityAffected(int teamId,
		int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getHighTrajectory(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMyGravity(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoExplode(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getStartVelocity(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getWeaponAcceleration(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTurnRate(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMaxVelocity(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getProjectileSpeed(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getExplosionSpeed(int teamId,
		int weaponDefId);
EXPORT(unsigned int     ) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getWobble(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDance(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTrajectoryHeight(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isLargeBeamLaser(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isShieldRepulser(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSmartShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isExteriorShield(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isVisibleShield(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isVisibleShieldRepulse(int teamId,
		int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse(
		int teamId, int weaponDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getRadius(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getForce(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getMaxSpeed(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPower(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPowerRegen(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource(
		int teamId, int weaponDefId, int resourceId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int teamId,
		int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int teamId,
		int weaponDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_WeaponDef_Shield_getGoodColor(int teamId,
		int weaponDefId);
EXPORT(struct SAIFloat3 ) skirmishAiCallback_WeaponDef_Shield_getBadColor(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getAlpha(int teamId,
		int weaponDefId);
EXPORT(unsigned int     ) skirmishAiCallback_WeaponDef_Shield_getInterceptType(int teamId,
		int weaponDefId);
EXPORT(unsigned int     ) skirmishAiCallback_WeaponDef_getInterceptedByShieldType(
		int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidFriendly(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidFeature(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidNeutral(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getTargetBorder(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCylinderTargetting(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getMinIntensity(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getHeightBoostFactor(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getProximityPriority(int teamId,
		int weaponDefId);
EXPORT(unsigned int     ) skirmishAiCallback_WeaponDef_getCollisionFlags(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSweepFire(int teamId, int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAbleToAttackGround(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getCameraShake(int teamId, int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageExp(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageMin(int teamId,
		int weaponDefId);
EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageRange(int teamId,
		int weaponDefId);
EXPORT(bool             ) skirmishAiCallback_WeaponDef_isDynDamageInverted(int teamId,
		int weaponDefId);
EXPORT(int              ) skirmishAiCallback_WeaponDef_0MAP1SIZE0getCustomParams(int teamId,
		int weaponDefId);
EXPORT(void) skirmishAiCallback_WeaponDef_0MAP1KEYS0getCustomParams(int teamId,
		int weaponDefId, const char* keys[]);
EXPORT(void) skirmishAiCallback_WeaponDef_0MAP1VALS0getCustomParams(int teamId,
		int weaponDefId, const char* values[]);
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
