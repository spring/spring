/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S_SKIRMISH_AI_CALLBACK_IMPL_H
#define	S_SKIRMISH_AI_CALLBACK_IMPL_H

// Doc-comments for the functions in this header can be found in this file:
// rts/ExternalAI/Interface/SSkirmishAICallback.h

#include "System/exportdefines.h"

#if	defined(__cplusplus)
extern "C" {
#endif

EXPORT(int              ) skirmishAiCallback_Engine_handleCommand(int skirmishAIId, int toId, int commandId, int commandTopic, void* commandData);

EXPORT(int              ) skirmishAiCallback_Engine_executeCommand(int skirmishAIId, int unitId, int groupId, void* commandData);


EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getFull(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getMajor(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getMinor(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getPatchset(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getCommits(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getHash(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getBranch(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getAdditional(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getBuildTime(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Engine_Version_isRelease(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getNormal(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getSync(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Engine_Version_getFull(int skirmishAIId);


EXPORT(int              ) skirmishAiCallback_Teams_getSize(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_SkirmishAIs_getSize(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_SkirmishAIs_getMax(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_SkirmishAI_getTeamId(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_SkirmishAI_Info_getSize(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getKey(int skirmishAIId, int infoIndex);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getValue(int skirmishAIId, int infoIndex);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getDescription(int skirmishAIId, int infoIndex);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_Info_getValueByKey(int skirmishAIId, const char* const key);

EXPORT(int              ) skirmishAiCallback_SkirmishAI_OptionValues_getSize(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_OptionValues_getKey(int skirmishAIId, int optionIndex);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_OptionValues_getValue(int skirmishAIId, int optionIndex);

EXPORT(const char*      ) skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey(int skirmishAIId, const char* const key);

EXPORT(void             ) skirmishAiCallback_Log_log(int skirmishAIId, const char* const msg);

EXPORT(void             ) skirmishAiCallback_Log_exception(int skirmishAIId, const char* const msg, int severety, bool die);

EXPORT(char             ) skirmishAiCallback_DataDirs_getPathSeparator(int UNUSED_skirmishAIId);

EXPORT(int              ) skirmishAiCallback_DataDirs_Roots_getSize(int UNUSED_skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_DataDirs_Roots_getDir(int UNUSED_skirmishAIId, char* path, int path_sizeMax, int dirIndex);

EXPORT(bool             ) skirmishAiCallback_DataDirs_Roots_locatePath(int UNUSED_skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);

EXPORT(char*            ) skirmishAiCallback_DataDirs_Roots_allocatePath(int UNUSED_skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir);

EXPORT(const char*      ) skirmishAiCallback_DataDirs_getConfigDir(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_DataDirs_locatePath(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);

EXPORT(char*            ) skirmishAiCallback_DataDirs_allocatePath(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir, bool common);

EXPORT(const char*      ) skirmishAiCallback_DataDirs_getWriteableDir(int skirmishAIId);

// BEGINN misc callback functions
EXPORT(int              ) skirmishAiCallback_Game_getCurrentFrame(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Game_getAiInterfaceVersion(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Game_getMyTeam(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Game_getMyAllyTeam(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Game_getPlayerTeam(int skirmishAIId, int playerId);

EXPORT(const char*      ) skirmishAiCallback_Game_getTeamSide(int skirmishAIId, int otherTeamId);

EXPORT(void             ) skirmishAiCallback_Game_getTeamColor(int skirmishAIId, int otherTeamId, short* return_colorS3_out);

EXPORT(int              ) skirmishAiCallback_Game_getTeamAllyTeam(int skirmishAIId, int otherTeamId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceCurrent(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceIncome(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceUsage(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceStorage(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourcePull(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceShare(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceSent(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceReceived(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Game_getTeamResourceExcess(int skirmishAIId, int otherTeamId, int resourceId);

EXPORT(bool             ) skirmishAiCallback_Game_isAllied(int skirmishAIId, int firstAllyTeamId, int secondAllyTeamId);

EXPORT(bool             ) skirmishAiCallback_Game_isDebugModeEnabled(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Game_isPaused(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Game_getSpeedFactor(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Game_getSetupScript(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Game_getCategoryFlag(int skirmishAIId, const char* categoryName);

EXPORT(int              ) skirmishAiCallback_Game_getCategoriesFlag(int skirmishAIId, const char* categoryNames);

EXPORT(void             ) skirmishAiCallback_Game_getCategoryName(int skirmishAIId, int categoryFlag, char* name, int name_sizeMax);

EXPORT(float            ) skirmishAiCallback_Game_getRulesParamFloat(int skirmishAIId, const char* rulesParamName, float defaultValue);

EXPORT(const char*      ) skirmishAiCallback_Game_getRulesParamString(int skirmishAIId, const char* rulesParamName, const char* defaultValue);

// END misc callback functions


// BEGINN Visualization related callback functions
EXPORT(float            ) skirmishAiCallback_Gui_getViewRange(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Gui_getScreenX(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Gui_getScreenY(int skirmishAIId);

EXPORT(void             ) skirmishAiCallback_Gui_Camera_getDirection(int skirmishAIId, float* return_posF3_out);

EXPORT(void             ) skirmishAiCallback_Gui_Camera_getPosition(int skirmishAIId, float* return_posF3_out);

// END Visualization related callback functions


// BEGINN OBJECT Cheats
EXPORT(bool             ) skirmishAiCallback_Cheats_isEnabled(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Cheats_setEnabled(int skirmishAIId, bool enable);

EXPORT(bool             ) skirmishAiCallback_Cheats_setEventsEnabled(int skirmishAIId, bool enabled);

EXPORT(bool             ) skirmishAiCallback_Cheats_isOnlyPassive(int skirmishAIId);

// END OBJECT Cheats


// BEGINN OBJECT Resource
EXPORT(int              ) skirmishAiCallback_getResources(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_getResourceByName(int skirmishAIId, const char* resourceName);

EXPORT(const char*      ) skirmishAiCallback_Resource_getName(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Resource_getOptimum(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getCurrent(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getIncome(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getUsage(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getStorage(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getPull(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getShare(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getSent(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getReceived(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Economy_getExcess(int skirmishAIId, int resourceId);

// END OBJECT Resource


// BEGINN OBJECT File
EXPORT(int              ) skirmishAiCallback_File_getSize(int skirmishAIId, const char* fileName);

EXPORT(bool             ) skirmishAiCallback_File_getContent(int skirmishAIId, const char* fileName, void* buffer, int bufferLen);

// EXPORT(bool             ) skirmishAiCallback_File_locateForReading(int skirmishAIId, char* fileName, int fileName_sizeMax);

// EXPORT(bool             ) skirmishAiCallback_File_locateForWriting(int skirmishAIId, char* fileName, int fileName_sizeMax);

// END OBJECT File


// BEGINN OBJECT UnitDef
EXPORT(int              ) skirmishAiCallback_getUnitDefs(int skirmishAIId, int* unitDefIds, int unitDefIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getUnitDefByName(int skirmishAIId, const char* unitName);

//int) skirmishAiCallback_UnitDef_getId(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getHeight(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getRadius(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getName(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getHumanName(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getFileName(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getAiHint(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getCobId(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getTechLevel(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getGaia(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getUpkeep(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getResourceMake(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMakesResource(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getCost(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getExtractsResource(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getResourceExtractorRange(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getWindResourceGenerator(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTidalResourceGenerator(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getStorage(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isSquareResourceExtractor(int skirmishAIId, int unitDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildTime(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getAutoHeal(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getIdleAutoHeal(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getIdleTime(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getPower(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getHealth(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getCategory(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnRate(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isTurnInPlace(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnInPlaceDistance(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit(int skirmishAIId, int unitDefId);

//EXPORT(int              ) skirmishAiCallback_UnitDef_getMoveType(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isUpright(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isCollide(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getLosRadius(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getAirLosRadius(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getLosHeight(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getRadarRadius(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getSonarRadius(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getJammerRadius(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getSonarJamRadius(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getSeismicRadius(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getSeismicSignature(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isStealth(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isSonarStealth(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isBuildRange3D(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildDistance(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getReclaimSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getRepairSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxRepairSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getResurrectSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getCaptureSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTerraformSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMass(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isPushResistant(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isStrafeToAttack(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMinCollisionSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getSlideTolerance(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxSlope(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxHeightDif(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMinWaterDepth(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getWaterline(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxWaterDepth(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getArmoredMultiple(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getArmorType(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_FlankingBonus_getMode(int skirmishAIId, int unitDefId);

EXPORT(void             ) skirmishAiCallback_UnitDef_FlankingBonus_getDir(int skirmishAIId, int unitDefId, float* return_posF3_out);

EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMax(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMin(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxWeaponRange(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getType(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getTooltip(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getWreckName(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getDeathExplosion(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getSelfDExplosion(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_getCategoryString(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToSelfD(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getSelfDCountdown(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToSubmerge(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToFly(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToMove(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToHover(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isFloater(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isBuilder(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isActivateWhenBuilt(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isOnOffable(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isFullHealthFactory(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isReclaimable(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isCapturable(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToRestore(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToRepair(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToSelfRepair(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToReclaim(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToAttack(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToPatrol(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToFight(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToGuard(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToAssist(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAssistable(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToRepeat(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToFireControl(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getFireState(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getMoveState(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getWingDrag(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getWingAngle(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getDrag(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getFrontToSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getSpeedToFront(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMyGravity(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxBank(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxPitch(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTurnRadius(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getWantedHeight(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getVerticalSpeed(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToCrash(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isHoverAttack(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAirStrafe(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getDlHoverFactor(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxAcceleration(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxDeceleration(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxAileron(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxElevator(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMaxRudder(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getYardMap(int skirmishAIId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax);

EXPORT(int              ) skirmishAiCallback_UnitDef_getXSize(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getZSize(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildAngle(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getLoadingRadius(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getUnloadSpread(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportCapacity(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportSize(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getMinTransportSize(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAirBase(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isFirePlatform(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTransportMass(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getMinTransportMass(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isHoldSteady(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isReleaseHeld(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isNotTransportable(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isTransportByEnemy(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getTransportUnloadMethod(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getFallSpeed(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getUnitFallSpeed(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToCloak(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isStartCloaked(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getCloakCost(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getCloakCostMoving(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getDecloakDistance(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isDecloakSpherical(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isDecloakOnFire(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToKamikaze(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getKamikazeDist(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isTargetingFacility(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_canManualFire(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isNeedGeo(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isFeature(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isHideDamage(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isCommander(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isShowPlayerName(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToResurrect(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToCapture(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getHighTrajectoryType(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getNoChaseCategory(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isLeaveTracks(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackWidth(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackOffset(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackStrength(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getTrackStretch(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getTrackType(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToDropFlare(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getFlareReloadTime(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getFlareEfficiency(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getFlareDelay(int skirmishAIId, int unitDefId);

EXPORT(void             ) skirmishAiCallback_UnitDef_getFlareDropVector(int skirmishAIId, int unitDefId, float* return_posF3_out);

EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareTime(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareSalvoSize(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getFlareSalvoDelay(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isAbleToLoopbackAttack(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isLevelGround(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isUseBuildingGroundDecal(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalType(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalSizeX(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildingDecalSizeY(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getMaxThisUnit(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getDecoyDef(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isDontLand(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getShieldDef(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getStockpileDef(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getBuildOptions(int skirmishAIId, int unitDefId, int* unitDefIds, int unitDefIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_UnitDef_getCustomParams(int skirmishAIId, int unitDefId, const char** keys, const char** values);

EXPORT(bool             ) skirmishAiCallback_UnitDef_isMoveDataAvailable(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getMoveType(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getSpeedModClass(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getXSize(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getZSize(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getDepth(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxSlope(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getSlopeMod(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getDepthMod(int skirmishAIId, int unitDefId, float height);

EXPORT(int              ) skirmishAiCallback_UnitDef_MoveData_getPathType(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getCrushStrength(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxSpeed(int skirmishAIId, int unitDefId);

EXPORT(short            ) skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration(int skirmishAIId, int unitDefId);

EXPORT(float            ) skirmishAiCallback_UnitDef_MoveData_getMaxBreaking(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_MoveData_getFollowGround(int skirmishAIId, int unitDefId);

EXPORT(bool             ) skirmishAiCallback_UnitDef_MoveData_isSubMarine(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_MoveData_getName(int skirmishAIId, int unitDefId);

EXPORT(int              ) skirmishAiCallback_UnitDef_getWeaponMounts(int skirmishAIId, int unitDefId);

EXPORT(const char*      ) skirmishAiCallback_UnitDef_WeaponMount_getName(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getWeaponDef(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(void             ) skirmishAiCallback_UnitDef_WeaponMount_getMainDir(int skirmishAIId, int unitDefId, int weaponMountId, float* return_posF3_out);

EXPORT(float            ) skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId);

EXPORT(int              ) skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId);

// END OBJECT UnitDef



// BEGINN OBJECT Unit
EXPORT(int              ) skirmishAiCallback_Unit_getLimit(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Unit_getMax(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_getEnemyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getEnemyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getEnemyUnitsInRadarAndLos(int skirmishAIId, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getFriendlyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getFriendlyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getNeutralUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getNeutralUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getTeamUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getSelectedUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_Unit_getDef(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getRulesParamFloat(int skirmishAIId, int unitId, const char* rulesParamName, float defaultValue);

EXPORT(const char*      ) skirmishAiCallback_Unit_getRulesParamString(int skirmishAIId, int unitId, const char* rulesParamName, const char* defaultValue);

EXPORT(int              ) skirmishAiCallback_Unit_getTeam(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getAllyTeam(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getAiHint(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getStockpile(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getStockpileQueued(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getMaxSpeed(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getMaxRange(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getMaxHealth(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getExperience(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getGroup(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getCurrentCommands(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getType(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getId(int skirmishAIId, int unitId, int commandId);

EXPORT(short            ) skirmishAiCallback_Unit_CurrentCommand_getOptions(int skirmishAIId, int unitId, int commandId);

EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getTag(int skirmishAIId, int unitId, int commandId);

EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int skirmishAIId, int unitId, int commandId);

EXPORT(int              ) skirmishAiCallback_Unit_CurrentCommand_getParams(int skirmishAIId, int unitId, int commandId, float* params, int params_sizeMax);

EXPORT(int              ) skirmishAiCallback_Unit_getSupportedCommands(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_getId(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(const char*      ) skirmishAiCallback_Unit_SupportedCommand_getName(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(const char*      ) skirmishAiCallback_Unit_SupportedCommand_getToolTip(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(bool             ) skirmishAiCallback_Unit_SupportedCommand_isShowUnique(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(bool             ) skirmishAiCallback_Unit_SupportedCommand_isDisabled(int skirmishAIId, int unitId, int supportedCommandId);

EXPORT(int              ) skirmishAiCallback_Unit_SupportedCommand_getParams(int skirmishAIId, int unitId, int supportedCommandId, const char** params, int params_sizeMax);

EXPORT(float            ) skirmishAiCallback_Unit_getHealth(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getSpeed(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getPower(int skirmishAIId, int unitId);

//EXPORT(int              ) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int skirmishAIId, int unitId);

EXPORT(float            ) skirmishAiCallback_Unit_getResourceUse(int skirmishAIId, int unitId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Unit_getResourceMake(int skirmishAIId, int unitId, int resourceId);

EXPORT(void             ) skirmishAiCallback_Unit_getPos(int skirmishAIId, int unitId, float* return_posF3_out);

EXPORT(void             ) skirmishAiCallback_Unit_getVel(int skirmishAIId, int unitId, float* return_posF3_out);

EXPORT(bool             ) skirmishAiCallback_Unit_isActivated(int skirmishAIId, int unitId);

EXPORT(bool             ) skirmishAiCallback_Unit_isBeingBuilt(int skirmishAIId, int unitId);

EXPORT(bool             ) skirmishAiCallback_Unit_isCloaked(int skirmishAIId, int unitId);

EXPORT(bool             ) skirmishAiCallback_Unit_isParalyzed(int skirmishAIId, int unitId);

EXPORT(bool             ) skirmishAiCallback_Unit_isNeutral(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getBuildingFacing(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getLastUserOrderFrame(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getWeapons(int skirmishAIId, int unitId);

EXPORT(int              ) skirmishAiCallback_Unit_getWeapon(int skirmishAIId, int unitId, int weaponMountId);

// END OBJECT Unit


// BEGINN OBJECT Team
EXPORT(bool             ) skirmishAiCallback_Team_hasAIController(int skirmishAIId, int teamId);

EXPORT(int              ) skirmishAiCallback_getEnemyTeams(int skirmishAIId, int* teamIds, int teamIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getAllyTeams(int skirmishAIId, int* teamIds, int teamIds_sizeMax);

EXPORT(float            ) skirmishAiCallback_Team_getRulesParamFloat(int skirmishAIId, int teamId, const char* rulesParamName, float defaultValue);

EXPORT(const char*      ) skirmishAiCallback_Team_getRulesParamString(int skirmishAIId, int teamId, const char* rulesParamName, const char* defaultValue);

// END OBJECT Team


// BEGINN OBJECT Group
EXPORT(int              ) skirmishAiCallback_getGroups(int skirmishAIId, int* groupIds, int groupIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_Group_getSupportedCommands(int skirmishAIId, int groupId);

EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_getId(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(const char*      ) skirmishAiCallback_Group_SupportedCommand_getName(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(const char*      ) skirmishAiCallback_Group_SupportedCommand_getToolTip(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(bool             ) skirmishAiCallback_Group_SupportedCommand_isShowUnique(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(bool             ) skirmishAiCallback_Group_SupportedCommand_isDisabled(int skirmishAIId, int groupId, int supportedCommandId);

EXPORT(int              ) skirmishAiCallback_Group_SupportedCommand_getParams(int skirmishAIId, int groupId, int supportedCommandId, const char** params, int params_sizeMax);

EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getId(int skirmishAIId, int groupId);

EXPORT(short            ) skirmishAiCallback_Group_OrderPreview_getOptions(int skirmishAIId, int groupId);

EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getTag(int skirmishAIId, int groupId);

EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getTimeOut(int skirmishAIId, int groupId);

EXPORT(int              ) skirmishAiCallback_Group_OrderPreview_getParams(int skirmishAIId, int groupId, float* params, int params_sizeMax);

EXPORT(bool             ) skirmishAiCallback_Group_isSelected(int skirmishAIId, int groupId);

// END OBJECT Group



// BEGINN OBJECT Mod
EXPORT(const char*      ) skirmishAiCallback_Mod_getFileName(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getHash(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Mod_getHumanName(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Mod_getShortName(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Mod_getVersion(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Mod_getMutator(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Mod_getDescription(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Mod_getAllowTeamColors(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Mod_getConstructionDecay(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getConstructionDecayTime(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getConstructionDecaySpeed(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getMultiReclaim(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getReclaimMethod(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getReclaimUnitMethod(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getReclaimUnitEnergyCostFactor(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getReclaimUnitEfficiency(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getReclaimFeatureEnergyCostFactor(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Mod_getReclaimAllowEnemies(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Mod_getReclaimAllowAllies(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getRepairEnergyCostFactor(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getResurrectEnergyCostFactor(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Mod_getCaptureEnergyCostFactor(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getTransportGround(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getTransportHover(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getTransportShip(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getTransportAir(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getFireAtKilled(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getFireAtCrashing(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getFlankingBonusModeDefault(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getLosMipLevel(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getAirMipLevel(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Mod_getRadarMipLevel(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Mod_getRequireSonarUnderWater(int skirmishAIId);

// END OBJECT Mod



// BEGINN OBJECT Map
EXPORT(int              ) skirmishAiCallback_Map_getChecksum(int skirmishAIId);

EXPORT(void             ) skirmishAiCallback_Map_getStartPos(int skirmishAIId, float* return_posF3_out);

EXPORT(void             ) skirmishAiCallback_Map_getMousePos(int skirmishAIId, float* return_posF3_out);

EXPORT(bool             ) skirmishAiCallback_Map_isPosInCamera(int skirmishAIId, float* pos_posF3, float radius);

EXPORT(int              ) skirmishAiCallback_Map_getWidth(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Map_getHeight(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Map_getHeightMap(int skirmishAIId, float* heights, int heights_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getCornersHeightMap(int skirmishAIId, float* cornerHeights, int cornerHeights_sizeMax);

EXPORT(float            ) skirmishAiCallback_Map_getMinHeight(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getMaxHeight(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Map_getSlopeMap(int skirmishAIId, float* slopes, int slopes_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getLosMap(int skirmishAIId, int* losValues, int losValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getAirLosMap(int skirmishAIId, int* airLosValues, int airLosValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getRadarMap(int skirmishAIId, int* radarValues, int radarValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getSonarMap(int skirmishAIId, int* sonarValues, int sonarValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getSeismicMap(int skirmishAIId, int* seismicValues, int seismicValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getJammerMap(int skirmishAIId, int* jammerValues, int jammerValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getSonarJammerMap(int skirmishAIId, int* sonarJammerValues, int sonarJammerValues_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getResourceMapRaw(int skirmishAIId, int resourceId, short* resources, int resources_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getResourceMapSpotsPositions(int skirmishAIId, int resourceId, float* spots_AposF3, int spots_AposF3_sizeMax);

EXPORT(float            ) skirmishAiCallback_Map_initResourceMapSpotsNearest(int skirmishAIId, int resourceId, float* pos_posF3, float* return_posF3_out);

EXPORT(int              ) skirmishAiCallback_Map_getHash(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Map_getName(int skirmishAIId);

EXPORT(const char*      ) skirmishAiCallback_Map_getHumanName(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getElevationAt(int skirmishAIId, float x, float z);

EXPORT(float            ) skirmishAiCallback_Map_getMaxResource(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Map_getExtractorRadius(int skirmishAIId, int resourceId);

EXPORT(float            ) skirmishAiCallback_Map_getMinWind(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getMaxWind(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getTidalStrength(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getGravity(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getWaterDamage(int skirmishAIId);

EXPORT(bool             ) skirmishAiCallback_Map_isDeformable(int skirmishAIId);

EXPORT(float            ) skirmishAiCallback_Map_getHardness(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_Map_getHardnessModMap(int skirmishAIId, float* hardMods, int hardMods_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getSpeedModMap(int skirmishAIId, int speedModClass, float* speedMods, int speedMods_sizeMax);

EXPORT(int              ) skirmishAiCallback_Map_getPoints(int skirmishAIId, bool includeAllies);

EXPORT(void             ) skirmishAiCallback_Map_Point_getPosition(int skirmishAIId, int pointId, float* return_posF3_out);

EXPORT(void             ) skirmishAiCallback_Map_Point_getColor(int skirmishAIId, int pointId, short* return_colorS3_out);

EXPORT(const char*      ) skirmishAiCallback_Map_Point_getLabel(int skirmishAIId, int pointId);

EXPORT(int              ) skirmishAiCallback_Map_getLines(int skirmishAIId, bool includeAllies);

EXPORT(void             ) skirmishAiCallback_Map_Line_getFirstPosition(int skirmishAIId, int lineId, float* return_posF3_out);

EXPORT(void             ) skirmishAiCallback_Map_Line_getSecondPosition(int skirmishAIId, int lineId, float* return_posF3_out);

EXPORT(void             ) skirmishAiCallback_Map_Line_getColor(int skirmishAIId, int lineId, short* return_colorS3_out);

EXPORT(bool             ) skirmishAiCallback_Map_isPossibleToBuildAt(int skirmishAIId, int unitDefId, float* pos_posF3, int facing);

EXPORT(void             ) skirmishAiCallback_Map_findClosestBuildSite(int skirmishAIId, int unitDefId, float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out);

// BEGINN OBJECT Map



// BEGINN OBJECT FeatureDef
EXPORT(int              ) skirmishAiCallback_getFeatureDefs(int skirmishAIId, int* featureDefIds, int featureDefIds_sizeMax);

//int) skirmishAiCallback_FeatureDef_getId(int skirmishAIId, int featureDefId);

EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getName(int skirmishAIId, int featureDefId);

EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getDescription(int skirmishAIId, int featureDefId);

EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getFileName(int skirmishAIId, int featureDefId);

EXPORT(float            ) skirmishAiCallback_FeatureDef_getContainedResource(int skirmishAIId, int featureDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_FeatureDef_getMaxHealth(int skirmishAIId, int featureDefId);

EXPORT(float            ) skirmishAiCallback_FeatureDef_getReclaimTime(int skirmishAIId, int featureDefId);

EXPORT(float            ) skirmishAiCallback_FeatureDef_getMass(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isUpright(int skirmishAIId, int featureDefId);

EXPORT(int              ) skirmishAiCallback_FeatureDef_getDrawType(int skirmishAIId, int featureDefId);

EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getModelName(int skirmishAIId, int featureDefId);

EXPORT(int              ) skirmishAiCallback_FeatureDef_getResurrectable(int skirmishAIId, int featureDefId);

EXPORT(int              ) skirmishAiCallback_FeatureDef_getSmokeTime(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isDestructable(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isReclaimable(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isBlocking(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isBurnable(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isFloating(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isNoSelect(int skirmishAIId, int featureDefId);

EXPORT(bool             ) skirmishAiCallback_FeatureDef_isGeoThermal(int skirmishAIId, int featureDefId);

EXPORT(const char*      ) skirmishAiCallback_FeatureDef_getDeathFeature(int skirmishAIId, int featureDefId);

EXPORT(int              ) skirmishAiCallback_FeatureDef_getXSize(int skirmishAIId, int featureDefId);

EXPORT(int              ) skirmishAiCallback_FeatureDef_getZSize(int skirmishAIId, int featureDefId);

EXPORT(int              ) skirmishAiCallback_FeatureDef_getCustomParams(int skirmishAIId, int featureDefId, const char** keys, const char** values);

// END OBJECT FeatureDef


// BEGINN OBJECT Feature
EXPORT(int              ) skirmishAiCallback_getFeatures(int skirmishAIId, int* featureIds, int featureIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_getFeaturesIn(int skirmishAIId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax);

EXPORT(int              ) skirmishAiCallback_Feature_getDef(int skirmishAIId, int featureId);

EXPORT(float            ) skirmishAiCallback_Feature_getHealth(int skirmishAIId, int featureId);

EXPORT(float            ) skirmishAiCallback_Feature_getReclaimLeft(int skirmishAIId, int featureId);

EXPORT(void             ) skirmishAiCallback_Feature_getPosition(int skirmishAIId, int featureId, float* return_posF3_out);

EXPORT(float            ) skirmishAiCallback_Feature_getRulesParamFloat(int skirmishAIId, int featureId, const char* rulesParamName, float defaultValue);

EXPORT(const char*      ) skirmishAiCallback_Feature_getRulesParamString(int skirmishAIId, int featureId, const char* rulesParamName, const char* defaultValue);

// END OBJECT Feature



// BEGINN OBJECT WeaponDef
EXPORT(int              ) skirmishAiCallback_getWeaponDefs(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_getWeaponDefByName(int skirmishAIId, const char* weaponDefName);

EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getName(int skirmishAIId, int weaponDefId);

EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getType(int skirmishAIId, int weaponDefId);

EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getDescription(int skirmishAIId, int weaponDefId);

EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getFileName(int skirmishAIId, int weaponDefId);

EXPORT(const char*      ) skirmishAiCallback_WeaponDef_getCegTag(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getRange(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getHeightMod(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getAccuracy(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getSprayAngle(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getMovingAccuracy(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getTargetMoveError(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getLeadLimit(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getLeadBonus(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getPredictBoost(int skirmishAIId, int weaponDefId);

// Deprecate the following function, if no longer needed by legacy Cpp AIs
EXPORT(int              ) skirmishAiCallback_WeaponDef_getNumDamageTypes(int skirmishAIId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getImpulseFactor(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getImpulseBoost(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getCraterMult(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getCraterBoost(int skirmishAIId, int weaponDefId);

//EXPORT(float            ) skirmishAiCallback_WeaponDef_Damage_getType(int skirmishAIId, int weaponDefId, int typeId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_Damage_getTypes(int skirmishAIId, int weaponDefId, float* types, int types_sizeMax);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getAreaOfEffect(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoSelfDamage(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getFireStarter(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getEdgeEffectiveness(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getSize(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getSizeGrowth(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getCollisionSize(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getSalvoSize(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getSalvoDelay(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getReload(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getBeamTime(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isBeamBurst(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isWaterBounce(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isGroundBounce(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getBounceRebound(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getBounceSlip(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getNumBounce(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getMaxAngle(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getUpTime(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getFlightTime(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getCost(int skirmishAIId, int weaponDefId, int resourceId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getProjectilesPerShot(int skirmishAIId, int weaponDefId);

//EXPORT(int              ) skirmishAiCallback_WeaponDef_getTdfId(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isTurret(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isOnlyForward(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isFixedLauncher(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isWaterWeapon(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isFireSubmersed(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSubMissile(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isTracks(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isDropped(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isParalyzer(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isImpactOnly(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoAutoTarget(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isManualFire(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getInterceptor(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getTargetable(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isStockpileable(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getCoverageRange(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getStockpileTime(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getIntensity(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getThickness(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getLaserFlareSize(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getCoreThickness(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getDuration(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getLodDistance(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getFalloffRate(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getGraphicsType(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSoundTrigger(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSelfExplode(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isGravityAffected(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getHighTrajectory(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getMyGravity(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isNoExplode(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getStartVelocity(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getWeaponAcceleration(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getTurnRate(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getMaxVelocity(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getProjectileSpeed(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getExplosionSpeed(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getWobble(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getDance(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getTrajectoryHeight(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isLargeBeamLaser(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isShield(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isShieldRepulser(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSmartShield(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isExteriorShield(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isVisibleShield(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isVisibleShieldRepulse(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getResourceUse(int skirmishAIId, int weaponDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getRadius(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getForce(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getMaxSpeed(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPower(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPowerRegen(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getPowerRegenResource(int skirmishAIId, int weaponDefId, int resourceId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int skirmishAIId, int weaponDefId);

EXPORT(void             ) skirmishAiCallback_WeaponDef_Shield_getGoodColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out);

EXPORT(void             ) skirmishAiCallback_WeaponDef_Shield_getBadColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out);

EXPORT(short            ) skirmishAiCallback_WeaponDef_Shield_getAlpha(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_Shield_getInterceptType(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getInterceptedByShieldType(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidFriendly(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidFeature(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAvoidNeutral(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getTargetBorder(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getCylinderTargetting(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getMinIntensity(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getHeightBoostFactor(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getProximityPriority(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getCollisionFlags(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isSweepFire(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isAbleToAttackGround(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getCameraShake(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageExp(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageMin(int skirmishAIId, int weaponDefId);

EXPORT(float            ) skirmishAiCallback_WeaponDef_getDynDamageRange(int skirmishAIId, int weaponDefId);

EXPORT(bool             ) skirmishAiCallback_WeaponDef_isDynDamageInverted(int skirmishAIId, int weaponDefId);

EXPORT(int              ) skirmishAiCallback_WeaponDef_getCustomParams(int skirmishAIId, int weaponDefId, const char** keys, const char** values);

// END OBJECT WeaponDef


// BEGINN OBJECT Weapon
EXPORT(int              ) skirmishAiCallback_Unit_Weapon_getDef(int skirmishAIId, int unitId, int weaponId);

EXPORT(int              ) skirmishAiCallback_Unit_Weapon_getReloadFrame(int skirmishAIId, int unitId, int weaponId);

EXPORT(int              ) skirmishAiCallback_Unit_Weapon_getReloadTime(int skirmishAIId, int unitId, int weaponId);

EXPORT(float            ) skirmishAiCallback_Unit_Weapon_getRange(int skirmishAIId, int unitId, int weaponId);

EXPORT(bool             ) skirmishAiCallback_Unit_Weapon_isShieldEnabled(int skirmishAIId, int unitId, int weaponId);

EXPORT(float            ) skirmishAiCallback_Unit_Weapon_getShieldPower(int skirmishAIId, int unitId, int weaponId);

// END OBJECT Weapon

EXPORT(bool             ) skirmishAiCallback_Debug_GraphDrawer_isEnabled(int skirmishAIId);

#if	defined(__cplusplus)
} // extern "C"
#endif

#if defined __cplusplus && !defined BUILDING_AI
struct SSkirmishAICallback;
class CAICallback;
class CAICheats;

// for engine internal use only

/**
 * Create the C Skirmish AI callback instance for a specific AI.
 * @see skirmishAiCallback_release
 */
SSkirmishAICallback* skirmishAiCallback_getInstanceFor(int skirmishAIId, int teamId, CAICallback* aiCallback, CAICheats* aiCheats);

/**
 * Releases the C Skirmish AI callback instance for a specific AI.
 * @see skirmishAiCallback_getInstanceFor
 */
void skirmishAiCallback_release(int skirmishAIId);

#endif // defined __cplusplus && !defined BUILDING_AI

#endif // S_SKIRMISH_AI_CALLBACK_IMPL_H
