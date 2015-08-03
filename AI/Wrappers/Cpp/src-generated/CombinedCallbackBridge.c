/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "CombinedCallbackBridge.h"

#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/AISCommands.h"


#define id_clb_sizeMax 8192
static const struct SSkirmishAICallback* id_clb[id_clb_sizeMax];

void funcPntBrdg_addCallback(const size_t skirmishAIId, const struct SSkirmishAICallback* clb) {
	//assert(skirmishAIId < id_clb_sizeMax);
	id_clb[skirmishAIId] = clb;
}
void funcPntBrdg_removeCallback(const size_t skirmishAIId) {
	//assert(skirmishAIId < id_clb_sizeMax);
	id_clb[skirmishAIId] = NULL;
}

EXPORT(const char*) bridged_Engine_Version_getMajor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getMajor(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getMinor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getMinor(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getPatchset(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getPatchset(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getCommits(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getCommits(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getHash(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getHash(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getBranch(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getBranch(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getAdditional(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getAdditional(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getBuildTime(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getBuildTime(skirmishAIId);
}
EXPORT(bool) bridged_Engine_Version_isRelease(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_isRelease(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getNormal(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getNormal(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getSync(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getSync(skirmishAIId);
}
EXPORT(const char*) bridged_Engine_Version_getFull(int skirmishAIId) {

	return id_clb[skirmishAIId]->Engine_Version_getFull(skirmishAIId);
}
EXPORT(int) bridged_Teams_getSize(int skirmishAIId) {

	return id_clb[skirmishAIId]->Teams_getSize(skirmishAIId);
}
EXPORT(int) bridged_SkirmishAIs_getSize(int skirmishAIId) {

	return id_clb[skirmishAIId]->SkirmishAIs_getSize(skirmishAIId);
}
EXPORT(int) bridged_SkirmishAIs_getMax(int skirmishAIId) {

	return id_clb[skirmishAIId]->SkirmishAIs_getMax(skirmishAIId);
}
EXPORT(int) bridged_SkirmishAI_getTeamId(int skirmishAIId) {

	return id_clb[skirmishAIId]->SkirmishAI_getTeamId(skirmishAIId);
}
EXPORT(int) bridged_SkirmishAI_Info_getSize(int skirmishAIId) {

	return id_clb[skirmishAIId]->SkirmishAI_Info_getSize(skirmishAIId);
}
EXPORT(const char*) bridged_SkirmishAI_Info_getKey(int skirmishAIId, int infoIndex) {

	return id_clb[skirmishAIId]->SkirmishAI_Info_getKey(skirmishAIId, infoIndex);
}
EXPORT(const char*) bridged_SkirmishAI_Info_getValue(int skirmishAIId, int infoIndex) {

	return id_clb[skirmishAIId]->SkirmishAI_Info_getValue(skirmishAIId, infoIndex);
}
EXPORT(const char*) bridged_SkirmishAI_Info_getDescription(int skirmishAIId, int infoIndex) {

	return id_clb[skirmishAIId]->SkirmishAI_Info_getDescription(skirmishAIId, infoIndex);
}
EXPORT(const char*) bridged_SkirmishAI_Info_getValueByKey(int skirmishAIId, const char* const key) {

	return id_clb[skirmishAIId]->SkirmishAI_Info_getValueByKey(skirmishAIId, key);
}
EXPORT(int) bridged_SkirmishAI_OptionValues_getSize(int skirmishAIId) {

	return id_clb[skirmishAIId]->SkirmishAI_OptionValues_getSize(skirmishAIId);
}
EXPORT(const char*) bridged_SkirmishAI_OptionValues_getKey(int skirmishAIId, int optionIndex) {

	return id_clb[skirmishAIId]->SkirmishAI_OptionValues_getKey(skirmishAIId, optionIndex);
}
EXPORT(const char*) bridged_SkirmishAI_OptionValues_getValue(int skirmishAIId, int optionIndex) {

	return id_clb[skirmishAIId]->SkirmishAI_OptionValues_getValue(skirmishAIId, optionIndex);
}
EXPORT(const char*) bridged_SkirmishAI_OptionValues_getValueByKey(int skirmishAIId, const char* const key) {

	return id_clb[skirmishAIId]->SkirmishAI_OptionValues_getValueByKey(skirmishAIId, key);
}
EXPORT(void) bridged_Log_log(int skirmishAIId, const char* const msg) {

	id_clb[skirmishAIId]->Log_log(skirmishAIId, msg);
}
EXPORT(void) bridged_Log_exception(int skirmishAIId, const char* const msg, int severety, bool die) {

	id_clb[skirmishAIId]->Log_exception(skirmishAIId, msg, severety, die);
}
EXPORT(char) bridged_DataDirs_getPathSeparator(int skirmishAIId) {

	return id_clb[skirmishAIId]->DataDirs_getPathSeparator(skirmishAIId);
}
EXPORT(const char*) bridged_DataDirs_getConfigDir(int skirmishAIId) {

	return id_clb[skirmishAIId]->DataDirs_getConfigDir(skirmishAIId);
}
EXPORT(const char*) bridged_DataDirs_getWriteableDir(int skirmishAIId) {

	return id_clb[skirmishAIId]->DataDirs_getWriteableDir(skirmishAIId);
}
EXPORT(bool) bridged_DataDirs_locatePath(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	return id_clb[skirmishAIId]->DataDirs_locatePath(skirmishAIId, path, path_sizeMax, relPath, writeable, create, dir, common);
}
EXPORT(char*) bridged_DataDirs_allocatePath(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	return id_clb[skirmishAIId]->DataDirs_allocatePath(skirmishAIId, relPath, writeable, create, dir, common);
}
EXPORT(int) bridged_DataDirs_Roots_getSize(int skirmishAIId) {

	return id_clb[skirmishAIId]->DataDirs_Roots_getSize(skirmishAIId);
}
EXPORT(bool) bridged_DataDirs_Roots_getDir(int skirmishAIId, char* path, int path_sizeMax, int dirIndex) {

	return id_clb[skirmishAIId]->DataDirs_Roots_getDir(skirmishAIId, path, path_sizeMax, dirIndex);
}
EXPORT(bool) bridged_DataDirs_Roots_locatePath(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {

	return id_clb[skirmishAIId]->DataDirs_Roots_locatePath(skirmishAIId, path, path_sizeMax, relPath, writeable, create, dir);
}
EXPORT(char*) bridged_DataDirs_Roots_allocatePath(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir) {

	return id_clb[skirmishAIId]->DataDirs_Roots_allocatePath(skirmishAIId, relPath, writeable, create, dir);
}
EXPORT(int) bridged_Game_getCurrentFrame(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getCurrentFrame(skirmishAIId);
}
EXPORT(int) bridged_Game_getAiInterfaceVersion(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getAiInterfaceVersion(skirmishAIId);
}
EXPORT(int) bridged_Game_getMyTeam(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getMyTeam(skirmishAIId);
}
EXPORT(int) bridged_Game_getMyAllyTeam(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getMyAllyTeam(skirmishAIId);
}
EXPORT(int) bridged_Game_getPlayerTeam(int skirmishAIId, int playerId) {

	return id_clb[skirmishAIId]->Game_getPlayerTeam(skirmishAIId, playerId);
}
EXPORT(int) bridged_Game_getTeams(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getTeams(skirmishAIId);
}
EXPORT(const char*) bridged_Game_getTeamSide(int skirmishAIId, int otherTeamId) {

	return id_clb[skirmishAIId]->Game_getTeamSide(skirmishAIId, otherTeamId);
}
EXPORT(void) bridged_Game_getTeamColor(int skirmishAIId, int otherTeamId, short* return_colorS3_out) {

	id_clb[skirmishAIId]->Game_getTeamColor(skirmishAIId, otherTeamId, return_colorS3_out);
}
EXPORT(float) bridged_Game_getTeamIncomeMultiplier(int skirmishAIId, int otherTeamId) {

	return id_clb[skirmishAIId]->Game_getTeamIncomeMultiplier(skirmishAIId, otherTeamId);
}
EXPORT(int) bridged_Game_getTeamAllyTeam(int skirmishAIId, int otherTeamId) {

	return id_clb[skirmishAIId]->Game_getTeamAllyTeam(skirmishAIId, otherTeamId);
}
EXPORT(float) bridged_Game_getTeamResourceCurrent(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceCurrent(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceIncome(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceIncome(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceUsage(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceUsage(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceStorage(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceStorage(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourcePull(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourcePull(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceShare(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceShare(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceSent(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceSent(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceReceived(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceReceived(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(float) bridged_Game_getTeamResourceExcess(int skirmishAIId, int otherTeamId, int resourceId) {

	return id_clb[skirmishAIId]->Game_getTeamResourceExcess(skirmishAIId, otherTeamId, resourceId);
}
EXPORT(bool) bridged_Game_isAllied(int skirmishAIId, int firstAllyTeamId, int secondAllyTeamId) {

	return id_clb[skirmishAIId]->Game_isAllied(skirmishAIId, firstAllyTeamId, secondAllyTeamId);
}
EXPORT(bool) bridged_Game_isExceptionHandlingEnabled(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_isExceptionHandlingEnabled(skirmishAIId);
}
EXPORT(bool) bridged_Game_isDebugModeEnabled(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_isDebugModeEnabled(skirmishAIId);
}
EXPORT(int) bridged_Game_getMode(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getMode(skirmishAIId);
}
EXPORT(bool) bridged_Game_isPaused(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_isPaused(skirmishAIId);
}
EXPORT(float) bridged_Game_getSpeedFactor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getSpeedFactor(skirmishAIId);
}
EXPORT(const char*) bridged_Game_getSetupScript(int skirmishAIId) {

	return id_clb[skirmishAIId]->Game_getSetupScript(skirmishAIId);
}
EXPORT(int) bridged_Game_getCategoryFlag(int skirmishAIId, const char* categoryName) {

	return id_clb[skirmishAIId]->Game_getCategoryFlag(skirmishAIId, categoryName);
}
EXPORT(int) bridged_Game_getCategoriesFlag(int skirmishAIId, const char* categoryNames) {

	return id_clb[skirmishAIId]->Game_getCategoriesFlag(skirmishAIId, categoryNames);
}
EXPORT(void) bridged_Game_getCategoryName(int skirmishAIId, int categoryFlag, char* name, int name_sizeMax) {

	id_clb[skirmishAIId]->Game_getCategoryName(skirmishAIId, categoryFlag, name, name_sizeMax);
}
EXPORT(int) bridged_Game_getGameRulesParams(int skirmishAIId, int* paramIds, int paramIds_sizeMax) {

	return id_clb[skirmishAIId]->Game_getGameRulesParams(skirmishAIId, paramIds, paramIds_sizeMax);
}
EXPORT(int) bridged_Game_getGameRulesParamByName(int skirmishAIId, const char* rulesParamName) {

	return id_clb[skirmishAIId]->Game_getGameRulesParamByName(skirmishAIId, rulesParamName);
}
EXPORT(int) bridged_Game_getGameRulesParamById(int skirmishAIId, int rulesParamId) {

	return id_clb[skirmishAIId]->Game_getGameRulesParamById(skirmishAIId, rulesParamId);
}
EXPORT(const char*) bridged_GameRulesParam_getName(int skirmishAIId, int gameRulesParamId) {

	return id_clb[skirmishAIId]->GameRulesParam_getName(skirmishAIId, gameRulesParamId);
}
EXPORT(float) bridged_GameRulesParam_getValueFloat(int skirmishAIId, int gameRulesParamId) {

	return id_clb[skirmishAIId]->GameRulesParam_getValueFloat(skirmishAIId, gameRulesParamId);
}
EXPORT(const char*) bridged_GameRulesParam_getValueString(int skirmishAIId, int gameRulesParamId) {

	return id_clb[skirmishAIId]->GameRulesParam_getValueString(skirmishAIId, gameRulesParamId);
}
EXPORT(float) bridged_Gui_getViewRange(int skirmishAIId) {

	return id_clb[skirmishAIId]->Gui_getViewRange(skirmishAIId);
}
EXPORT(float) bridged_Gui_getScreenX(int skirmishAIId) {

	return id_clb[skirmishAIId]->Gui_getScreenX(skirmishAIId);
}
EXPORT(float) bridged_Gui_getScreenY(int skirmishAIId) {

	return id_clb[skirmishAIId]->Gui_getScreenY(skirmishAIId);
}
EXPORT(void) bridged_Gui_Camera_getDirection(int skirmishAIId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Gui_Camera_getDirection(skirmishAIId, return_posF3_out);
}
EXPORT(void) bridged_Gui_Camera_getPosition(int skirmishAIId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Gui_Camera_getPosition(skirmishAIId, return_posF3_out);
}
EXPORT(bool) bridged_Cheats_isEnabled(int skirmishAIId) {

	return id_clb[skirmishAIId]->Cheats_isEnabled(skirmishAIId);
}
EXPORT(bool) bridged_Cheats_setEnabled(int skirmishAIId, bool enable) {

	return id_clb[skirmishAIId]->Cheats_setEnabled(skirmishAIId, enable);
}
EXPORT(bool) bridged_Cheats_setEventsEnabled(int skirmishAIId, bool enabled) {

	return id_clb[skirmishAIId]->Cheats_setEventsEnabled(skirmishAIId, enabled);
}
EXPORT(bool) bridged_Cheats_isOnlyPassive(int skirmishAIId) {

	return id_clb[skirmishAIId]->Cheats_isOnlyPassive(skirmishAIId);
}
EXPORT(int) bridged_getResources(int skirmishAIId) {

	return id_clb[skirmishAIId]->getResources(skirmishAIId);
}
EXPORT(int) bridged_getResourceByName(int skirmishAIId, const char* resourceName) {

	return id_clb[skirmishAIId]->getResourceByName(skirmishAIId, resourceName);
}
EXPORT(const char*) bridged_Resource_getName(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Resource_getName(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Resource_getOptimum(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Resource_getOptimum(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getCurrent(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getCurrent(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getIncome(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getIncome(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getUsage(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getUsage(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getStorage(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getStorage(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getPull(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getPull(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getShare(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getShare(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getSent(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getSent(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getReceived(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getReceived(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Economy_getExcess(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Economy_getExcess(skirmishAIId, resourceId);
}
EXPORT(int) bridged_File_getSize(int skirmishAIId, const char* fileName) {

	return id_clb[skirmishAIId]->File_getSize(skirmishAIId, fileName);
}
EXPORT(bool) bridged_File_getContent(int skirmishAIId, const char* fileName, void* buffer, int bufferLen) {

	return id_clb[skirmishAIId]->File_getContent(skirmishAIId, fileName, buffer, bufferLen);
}
EXPORT(int) bridged_getUnitDefs(int skirmishAIId, int* unitDefIds, int unitDefIds_sizeMax) {

	return id_clb[skirmishAIId]->getUnitDefs(skirmishAIId, unitDefIds, unitDefIds_sizeMax);
}
EXPORT(int) bridged_getUnitDefByName(int skirmishAIId, const char* unitName) {

	return id_clb[skirmishAIId]->getUnitDefByName(skirmishAIId, unitName);
}
EXPORT(float) bridged_UnitDef_getHeight(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getHeight(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getRadius(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getName(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getName(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getHumanName(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getHumanName(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getFileName(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFileName(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getAiHint(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getAiHint(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getCobId(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getCobId(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getTechLevel(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTechLevel(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getGaia(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getGaia(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getUpkeep(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getUpkeep(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getResourceMake(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getResourceMake(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getMakesResource(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getMakesResource(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getCost(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getCost(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getExtractsResource(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getExtractsResource(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getResourceExtractorRange(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getResourceExtractorRange(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getWindResourceGenerator(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getWindResourceGenerator(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getTidalResourceGenerator(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getTidalResourceGenerator(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getStorage(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_getStorage(skirmishAIId, unitDefId, resourceId);
}
EXPORT(bool) bridged_UnitDef_isSquareResourceExtractor(int skirmishAIId, int unitDefId, int resourceId) {

	return id_clb[skirmishAIId]->UnitDef_isSquareResourceExtractor(skirmishAIId, unitDefId, resourceId);
}
EXPORT(float) bridged_UnitDef_getBuildTime(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildTime(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getAutoHeal(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getAutoHeal(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getIdleAutoHeal(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getIdleAutoHeal(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getIdleTime(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getIdleTime(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getPower(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getPower(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getHealth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getHealth(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getCategory(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getCategory(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTurnRate(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTurnRate(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isTurnInPlace(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isTurnInPlace(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTurnInPlaceDistance(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTurnInPlaceDistance(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTurnInPlaceSpeedLimit(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTurnInPlaceSpeedLimit(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isUpright(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isUpright(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isCollide(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isCollide(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getLosRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getLosRadius(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getAirLosRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getAirLosRadius(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getLosHeight(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getLosHeight(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getRadarRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getRadarRadius(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getSonarRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSonarRadius(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getJammerRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getJammerRadius(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getSonarJamRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSonarJamRadius(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getSeismicRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSeismicRadius(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getSeismicSignature(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSeismicSignature(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isStealth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isStealth(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isSonarStealth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isSonarStealth(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isBuildRange3D(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isBuildRange3D(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getBuildDistance(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildDistance(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getBuildSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getReclaimSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getReclaimSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getRepairSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getRepairSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxRepairSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxRepairSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getResurrectSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getResurrectSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getCaptureSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getCaptureSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTerraformSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTerraformSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMass(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMass(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isPushResistant(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isPushResistant(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isStrafeToAttack(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isStrafeToAttack(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMinCollisionSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMinCollisionSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getSlideTolerance(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSlideTolerance(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxSlope(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxSlope(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxHeightDif(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxHeightDif(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMinWaterDepth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMinWaterDepth(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getWaterline(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getWaterline(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxWaterDepth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxWaterDepth(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getArmoredMultiple(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getArmoredMultiple(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getArmorType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getArmorType(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_FlankingBonus_getMode(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_FlankingBonus_getMode(skirmishAIId, unitDefId);
}
EXPORT(void) bridged_UnitDef_FlankingBonus_getDir(int skirmishAIId, int unitDefId, float* return_posF3_out) {

	id_clb[skirmishAIId]->UnitDef_FlankingBonus_getDir(skirmishAIId, unitDefId, return_posF3_out);
}
EXPORT(float) bridged_UnitDef_FlankingBonus_getMax(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_FlankingBonus_getMax(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_FlankingBonus_getMin(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_FlankingBonus_getMin(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_FlankingBonus_getMobilityAdd(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_FlankingBonus_getMobilityAdd(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxWeaponRange(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxWeaponRange(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getType(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getTooltip(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTooltip(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getWreckName(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getWreckName(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getDeathExplosion(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getDeathExplosion(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getSelfDExplosion(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSelfDExplosion(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_getCategoryString(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getCategoryString(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToSelfD(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToSelfD(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getSelfDCountdown(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSelfDCountdown(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToSubmerge(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToSubmerge(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToFly(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToFly(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToMove(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToMove(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToHover(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToHover(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isFloater(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isFloater(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isBuilder(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isBuilder(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isActivateWhenBuilt(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isActivateWhenBuilt(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isOnOffable(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isOnOffable(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isFullHealthFactory(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isFullHealthFactory(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isFactoryHeadingTakeoff(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isFactoryHeadingTakeoff(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isReclaimable(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isReclaimable(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isCapturable(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isCapturable(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToRestore(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToRestore(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToRepair(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToRepair(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToSelfRepair(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToSelfRepair(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToReclaim(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToReclaim(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToAttack(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToAttack(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToPatrol(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToPatrol(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToFight(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToFight(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToGuard(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToGuard(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToAssist(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToAssist(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAssistable(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAssistable(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToRepeat(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToRepeat(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToFireControl(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToFireControl(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getFireState(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFireState(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getMoveState(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMoveState(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getWingDrag(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getWingDrag(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getWingAngle(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getWingAngle(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getDrag(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getDrag(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getFrontToSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFrontToSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getSpeedToFront(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getSpeedToFront(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMyGravity(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMyGravity(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxBank(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxBank(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxPitch(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxPitch(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTurnRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTurnRadius(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getWantedHeight(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getWantedHeight(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getVerticalSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getVerticalSpeed(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToCrash(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToCrash(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isHoverAttack(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isHoverAttack(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAirStrafe(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAirStrafe(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getDlHoverFactor(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getDlHoverFactor(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxAcceleration(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxAcceleration(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxDeceleration(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxDeceleration(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxAileron(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxAileron(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxElevator(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxElevator(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxRudder(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxRudder(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getYardMap(int skirmishAIId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax) {

	return id_clb[skirmishAIId]->UnitDef_getYardMap(skirmishAIId, unitDefId, facing, yardMap, yardMap_sizeMax);
}
EXPORT(int) bridged_UnitDef_getXSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getXSize(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getZSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getZSize(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getBuildAngle(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildAngle(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getLoadingRadius(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getLoadingRadius(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getUnloadSpread(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getUnloadSpread(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getTransportCapacity(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTransportCapacity(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getTransportSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTransportSize(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getMinTransportSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMinTransportSize(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAirBase(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAirBase(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isFirePlatform(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isFirePlatform(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTransportMass(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTransportMass(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMinTransportMass(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMinTransportMass(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isHoldSteady(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isHoldSteady(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isReleaseHeld(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isReleaseHeld(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isNotTransportable(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isNotTransportable(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isTransportByEnemy(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isTransportByEnemy(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getTransportUnloadMethod(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTransportUnloadMethod(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getFallSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFallSpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getUnitFallSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getUnitFallSpeed(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToCloak(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToCloak(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isStartCloaked(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isStartCloaked(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getCloakCost(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getCloakCost(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getCloakCostMoving(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getCloakCostMoving(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getDecloakDistance(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getDecloakDistance(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isDecloakSpherical(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isDecloakSpherical(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isDecloakOnFire(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isDecloakOnFire(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToKamikaze(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToKamikaze(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getKamikazeDist(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getKamikazeDist(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isTargetingFacility(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isTargetingFacility(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_canManualFire(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_canManualFire(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isNeedGeo(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isNeedGeo(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isFeature(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isFeature(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isHideDamage(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isHideDamage(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isCommander(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isCommander(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isShowPlayerName(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isShowPlayerName(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToResurrect(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToResurrect(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToCapture(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToCapture(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getHighTrajectoryType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getHighTrajectoryType(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getNoChaseCategory(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getNoChaseCategory(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isLeaveTracks(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isLeaveTracks(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTrackWidth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTrackWidth(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTrackOffset(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTrackOffset(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTrackStrength(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTrackStrength(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getTrackStretch(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTrackStretch(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getTrackType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getTrackType(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToDropFlare(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToDropFlare(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getFlareReloadTime(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFlareReloadTime(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getFlareEfficiency(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFlareEfficiency(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getFlareDelay(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFlareDelay(skirmishAIId, unitDefId);
}
EXPORT(void) bridged_UnitDef_getFlareDropVector(int skirmishAIId, int unitDefId, float* return_posF3_out) {

	id_clb[skirmishAIId]->UnitDef_getFlareDropVector(skirmishAIId, unitDefId, return_posF3_out);
}
EXPORT(int) bridged_UnitDef_getFlareTime(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFlareTime(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getFlareSalvoSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFlareSalvoSize(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getFlareSalvoDelay(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getFlareSalvoDelay(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isAbleToLoopbackAttack(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isAbleToLoopbackAttack(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isLevelGround(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isLevelGround(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isUseBuildingGroundDecal(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isUseBuildingGroundDecal(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getBuildingDecalType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildingDecalType(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getBuildingDecalSizeX(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildingDecalSizeX(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getBuildingDecalSizeY(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildingDecalSizeY(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getBuildingDecalDecaySpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getBuildingDecalDecaySpeed(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMaxFuel(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxFuel(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getRefuelTime(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getRefuelTime(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_getMinAirBasePower(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMinAirBasePower(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getMaxThisUnit(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getMaxThisUnit(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getDecoyDef(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getDecoyDef(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_isDontLand(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isDontLand(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getShieldDef(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getShieldDef(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getStockpileDef(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getStockpileDef(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getBuildOptions(int skirmishAIId, int unitDefId, int* unitDefIds, int unitDefIds_sizeMax) {

	return id_clb[skirmishAIId]->UnitDef_getBuildOptions(skirmishAIId, unitDefId, unitDefIds, unitDefIds_sizeMax);
}
EXPORT(int) bridged_UnitDef_getCustomParams(int skirmishAIId, int unitDefId, const char** keys, const char** values) {

	return id_clb[skirmishAIId]->UnitDef_getCustomParams(skirmishAIId, unitDefId, keys, values);
}
EXPORT(bool) bridged_UnitDef_isMoveDataAvailable(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_isMoveDataAvailable(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getMaxAcceleration(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getMaxAcceleration(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getMaxBreaking(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getMaxBreaking(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getMaxSpeed(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getMaxSpeed(skirmishAIId, unitDefId);
}
EXPORT(short) bridged_UnitDef_MoveData_getMaxTurnRate(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getMaxTurnRate(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_MoveData_getXSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getXSize(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_MoveData_getZSize(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getZSize(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getDepth(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getDepth(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getMaxSlope(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getMaxSlope(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getSlopeMod(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getSlopeMod(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getDepthMod(int skirmishAIId, int unitDefId, float height) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getDepthMod(skirmishAIId, unitDefId, height);
}
EXPORT(int) bridged_UnitDef_MoveData_getPathType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getPathType(skirmishAIId, unitDefId);
}
EXPORT(float) bridged_UnitDef_MoveData_getCrushStrength(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getCrushStrength(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_MoveData_getMoveType(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getMoveType(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_MoveData_getSpeedModClass(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getSpeedModClass(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_MoveData_getTerrainClass(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getTerrainClass(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_MoveData_getFollowGround(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getFollowGround(skirmishAIId, unitDefId);
}
EXPORT(bool) bridged_UnitDef_MoveData_isSubMarine(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_isSubMarine(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_MoveData_getName(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_MoveData_getName(skirmishAIId, unitDefId);
}
EXPORT(int) bridged_UnitDef_getWeaponMounts(int skirmishAIId, int unitDefId) {

	return id_clb[skirmishAIId]->UnitDef_getWeaponMounts(skirmishAIId, unitDefId);
}
EXPORT(const char*) bridged_UnitDef_WeaponMount_getName(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getName(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(int) bridged_UnitDef_WeaponMount_getWeaponDef(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getWeaponDef(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(int) bridged_UnitDef_WeaponMount_getSlavedTo(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getSlavedTo(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(void) bridged_UnitDef_WeaponMount_getMainDir(int skirmishAIId, int unitDefId, int weaponMountId, float* return_posF3_out) {

	id_clb[skirmishAIId]->UnitDef_WeaponMount_getMainDir(skirmishAIId, unitDefId, weaponMountId, return_posF3_out);
}
EXPORT(float) bridged_UnitDef_WeaponMount_getMaxAngleDif(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getMaxAngleDif(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(float) bridged_UnitDef_WeaponMount_getFuelUsage(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getFuelUsage(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(int) bridged_UnitDef_WeaponMount_getBadTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getBadTargetCategory(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(int) bridged_UnitDef_WeaponMount_getOnlyTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId) {

	return id_clb[skirmishAIId]->UnitDef_WeaponMount_getOnlyTargetCategory(skirmishAIId, unitDefId, weaponMountId);
}
EXPORT(int) bridged_Unit_getLimit(int skirmishAIId) {

	return id_clb[skirmishAIId]->Unit_getLimit(skirmishAIId);
}
EXPORT(int) bridged_Unit_getMax(int skirmishAIId) {

	return id_clb[skirmishAIId]->Unit_getMax(skirmishAIId);
}
EXPORT(int) bridged_getEnemyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getEnemyUnits(skirmishAIId, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getEnemyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getEnemyUnitsIn(skirmishAIId, pos_posF3, radius, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getEnemyUnitsInRadarAndLos(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getEnemyUnitsInRadarAndLos(skirmishAIId, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getFriendlyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getFriendlyUnits(skirmishAIId, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getFriendlyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getFriendlyUnitsIn(skirmishAIId, pos_posF3, radius, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getNeutralUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getNeutralUnits(skirmishAIId, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getNeutralUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getNeutralUnitsIn(skirmishAIId, pos_posF3, radius, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getTeamUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getTeamUnits(skirmishAIId, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_getSelectedUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	return id_clb[skirmishAIId]->getSelectedUnits(skirmishAIId, unitIds, unitIds_sizeMax);
}
EXPORT(int) bridged_Unit_getDef(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getDef(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getUnitRulesParams(int skirmishAIId, int unitId, int* paramIds, int paramIds_sizeMax) {

	return id_clb[skirmishAIId]->Unit_getUnitRulesParams(skirmishAIId, unitId, paramIds, paramIds_sizeMax);
}
EXPORT(int) bridged_Unit_getUnitRulesParamByName(int skirmishAIId, int unitId, const char* rulesParamName) {

	return id_clb[skirmishAIId]->Unit_getUnitRulesParamByName(skirmishAIId, unitId, rulesParamName);
}
EXPORT(int) bridged_Unit_getUnitRulesParamById(int skirmishAIId, int unitId, int rulesParamId) {

	return id_clb[skirmishAIId]->Unit_getUnitRulesParamById(skirmishAIId, unitId, rulesParamId);
}
EXPORT(const char*) bridged_Unit_UnitRulesParam_getName(int skirmishAIId, int unitId, int unitRulesParamId) {

	return id_clb[skirmishAIId]->Unit_UnitRulesParam_getName(skirmishAIId, unitId, unitRulesParamId);
}
EXPORT(float) bridged_Unit_UnitRulesParam_getValueFloat(int skirmishAIId, int unitId, int unitRulesParamId) {

	return id_clb[skirmishAIId]->Unit_UnitRulesParam_getValueFloat(skirmishAIId, unitId, unitRulesParamId);
}
EXPORT(const char*) bridged_Unit_UnitRulesParam_getValueString(int skirmishAIId, int unitId, int unitRulesParamId) {

	return id_clb[skirmishAIId]->Unit_UnitRulesParam_getValueString(skirmishAIId, unitId, unitRulesParamId);
}
EXPORT(int) bridged_Unit_getTeam(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getTeam(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getAllyTeam(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getAllyTeam(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getAiHint(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getAiHint(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getStockpile(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getStockpile(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getStockpileQueued(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getStockpileQueued(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getCurrentFuel(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getCurrentFuel(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getMaxSpeed(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getMaxSpeed(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getMaxRange(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getMaxRange(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getMaxHealth(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getMaxHealth(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getExperience(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getExperience(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getGroup(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getGroup(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getCurrentCommands(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getCurrentCommands(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_CurrentCommand_getType(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_CurrentCommand_getType(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_CurrentCommand_getId(int skirmishAIId, int unitId, int commandId) {

	return id_clb[skirmishAIId]->Unit_CurrentCommand_getId(skirmishAIId, unitId, commandId);
}
EXPORT(short) bridged_Unit_CurrentCommand_getOptions(int skirmishAIId, int unitId, int commandId) {

	return id_clb[skirmishAIId]->Unit_CurrentCommand_getOptions(skirmishAIId, unitId, commandId);
}
EXPORT(int) bridged_Unit_CurrentCommand_getTag(int skirmishAIId, int unitId, int commandId) {

	return id_clb[skirmishAIId]->Unit_CurrentCommand_getTag(skirmishAIId, unitId, commandId);
}
EXPORT(int) bridged_Unit_CurrentCommand_getTimeOut(int skirmishAIId, int unitId, int commandId) {

	return id_clb[skirmishAIId]->Unit_CurrentCommand_getTimeOut(skirmishAIId, unitId, commandId);
}
EXPORT(int) bridged_Unit_CurrentCommand_getParams(int skirmishAIId, int unitId, int commandId, float* params, int params_sizeMax) {

	return id_clb[skirmishAIId]->Unit_CurrentCommand_getParams(skirmishAIId, unitId, commandId, params, params_sizeMax);
}
EXPORT(int) bridged_Unit_getSupportedCommands(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getSupportedCommands(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_SupportedCommand_getId(int skirmishAIId, int unitId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Unit_SupportedCommand_getId(skirmishAIId, unitId, supportedCommandId);
}
EXPORT(const char*) bridged_Unit_SupportedCommand_getName(int skirmishAIId, int unitId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Unit_SupportedCommand_getName(skirmishAIId, unitId, supportedCommandId);
}
EXPORT(const char*) bridged_Unit_SupportedCommand_getToolTip(int skirmishAIId, int unitId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Unit_SupportedCommand_getToolTip(skirmishAIId, unitId, supportedCommandId);
}
EXPORT(bool) bridged_Unit_SupportedCommand_isShowUnique(int skirmishAIId, int unitId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Unit_SupportedCommand_isShowUnique(skirmishAIId, unitId, supportedCommandId);
}
EXPORT(bool) bridged_Unit_SupportedCommand_isDisabled(int skirmishAIId, int unitId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Unit_SupportedCommand_isDisabled(skirmishAIId, unitId, supportedCommandId);
}
EXPORT(int) bridged_Unit_SupportedCommand_getParams(int skirmishAIId, int unitId, int supportedCommandId, const char** params, int params_sizeMax) {

	return id_clb[skirmishAIId]->Unit_SupportedCommand_getParams(skirmishAIId, unitId, supportedCommandId, params, params_sizeMax);
}
EXPORT(float) bridged_Unit_getHealth(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getHealth(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getSpeed(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getSpeed(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getPower(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getPower(skirmishAIId, unitId);
}
EXPORT(float) bridged_Unit_getResourceUse(int skirmishAIId, int unitId, int resourceId) {

	return id_clb[skirmishAIId]->Unit_getResourceUse(skirmishAIId, unitId, resourceId);
}
EXPORT(float) bridged_Unit_getResourceMake(int skirmishAIId, int unitId, int resourceId) {

	return id_clb[skirmishAIId]->Unit_getResourceMake(skirmishAIId, unitId, resourceId);
}
EXPORT(void) bridged_Unit_getPos(int skirmishAIId, int unitId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Unit_getPos(skirmishAIId, unitId, return_posF3_out);
}
EXPORT(void) bridged_Unit_getVel(int skirmishAIId, int unitId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Unit_getVel(skirmishAIId, unitId, return_posF3_out);
}
EXPORT(bool) bridged_Unit_isActivated(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_isActivated(skirmishAIId, unitId);
}
EXPORT(bool) bridged_Unit_isBeingBuilt(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_isBeingBuilt(skirmishAIId, unitId);
}
EXPORT(bool) bridged_Unit_isCloaked(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_isCloaked(skirmishAIId, unitId);
}
EXPORT(bool) bridged_Unit_isParalyzed(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_isParalyzed(skirmishAIId, unitId);
}
EXPORT(bool) bridged_Unit_isNeutral(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_isNeutral(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getBuildingFacing(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getBuildingFacing(skirmishAIId, unitId);
}
EXPORT(int) bridged_Unit_getLastUserOrderFrame(int skirmishAIId, int unitId) {

	return id_clb[skirmishAIId]->Unit_getLastUserOrderFrame(skirmishAIId, unitId);
}
EXPORT(bool) bridged_Team_hasAIController(int skirmishAIId, int teamId) {

	return id_clb[skirmishAIId]->Team_hasAIController(skirmishAIId, teamId);
}
EXPORT(int) bridged_getEnemyTeams(int skirmishAIId, int* teamIds, int teamIds_sizeMax) {

	return id_clb[skirmishAIId]->getEnemyTeams(skirmishAIId, teamIds, teamIds_sizeMax);
}
EXPORT(int) bridged_getAllyTeams(int skirmishAIId, int* teamIds, int teamIds_sizeMax) {

	return id_clb[skirmishAIId]->getAllyTeams(skirmishAIId, teamIds, teamIds_sizeMax);
}
EXPORT(int) bridged_Team_getTeamRulesParams(int skirmishAIId, int teamId, int* paramIds, int paramIds_sizeMax) {

	return id_clb[skirmishAIId]->Team_getTeamRulesParams(skirmishAIId, teamId, paramIds, paramIds_sizeMax);
}
EXPORT(int) bridged_Team_getTeamRulesParamByName(int skirmishAIId, int teamId, const char* rulesParamName) {

	return id_clb[skirmishAIId]->Team_getTeamRulesParamByName(skirmishAIId, teamId, rulesParamName);
}
EXPORT(int) bridged_Team_getTeamRulesParamById(int skirmishAIId, int teamId, int rulesParamId) {

	return id_clb[skirmishAIId]->Team_getTeamRulesParamById(skirmishAIId, teamId, rulesParamId);
}
EXPORT(const char*) bridged_Team_TeamRulesParam_getName(int skirmishAIId, int teamId, int teamRulesParamId) {

	return id_clb[skirmishAIId]->Team_TeamRulesParam_getName(skirmishAIId, teamId, teamRulesParamId);
}
EXPORT(float) bridged_Team_TeamRulesParam_getValueFloat(int skirmishAIId, int teamId, int teamRulesParamId) {

	return id_clb[skirmishAIId]->Team_TeamRulesParam_getValueFloat(skirmishAIId, teamId, teamRulesParamId);
}
EXPORT(const char*) bridged_Team_TeamRulesParam_getValueString(int skirmishAIId, int teamId, int teamRulesParamId) {

	return id_clb[skirmishAIId]->Team_TeamRulesParam_getValueString(skirmishAIId, teamId, teamRulesParamId);
}
EXPORT(int) bridged_getGroups(int skirmishAIId, int* groupIds, int groupIds_sizeMax) {

	return id_clb[skirmishAIId]->getGroups(skirmishAIId, groupIds, groupIds_sizeMax);
}
EXPORT(int) bridged_Group_getSupportedCommands(int skirmishAIId, int groupId) {

	return id_clb[skirmishAIId]->Group_getSupportedCommands(skirmishAIId, groupId);
}
EXPORT(int) bridged_Group_SupportedCommand_getId(int skirmishAIId, int groupId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Group_SupportedCommand_getId(skirmishAIId, groupId, supportedCommandId);
}
EXPORT(const char*) bridged_Group_SupportedCommand_getName(int skirmishAIId, int groupId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Group_SupportedCommand_getName(skirmishAIId, groupId, supportedCommandId);
}
EXPORT(const char*) bridged_Group_SupportedCommand_getToolTip(int skirmishAIId, int groupId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Group_SupportedCommand_getToolTip(skirmishAIId, groupId, supportedCommandId);
}
EXPORT(bool) bridged_Group_SupportedCommand_isShowUnique(int skirmishAIId, int groupId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Group_SupportedCommand_isShowUnique(skirmishAIId, groupId, supportedCommandId);
}
EXPORT(bool) bridged_Group_SupportedCommand_isDisabled(int skirmishAIId, int groupId, int supportedCommandId) {

	return id_clb[skirmishAIId]->Group_SupportedCommand_isDisabled(skirmishAIId, groupId, supportedCommandId);
}
EXPORT(int) bridged_Group_SupportedCommand_getParams(int skirmishAIId, int groupId, int supportedCommandId, const char** params, int params_sizeMax) {

	return id_clb[skirmishAIId]->Group_SupportedCommand_getParams(skirmishAIId, groupId, supportedCommandId, params, params_sizeMax);
}
EXPORT(int) bridged_Group_OrderPreview_getId(int skirmishAIId, int groupId) {

	return id_clb[skirmishAIId]->Group_OrderPreview_getId(skirmishAIId, groupId);
}
EXPORT(short) bridged_Group_OrderPreview_getOptions(int skirmishAIId, int groupId) {

	return id_clb[skirmishAIId]->Group_OrderPreview_getOptions(skirmishAIId, groupId);
}
EXPORT(int) bridged_Group_OrderPreview_getTag(int skirmishAIId, int groupId) {

	return id_clb[skirmishAIId]->Group_OrderPreview_getTag(skirmishAIId, groupId);
}
EXPORT(int) bridged_Group_OrderPreview_getTimeOut(int skirmishAIId, int groupId) {

	return id_clb[skirmishAIId]->Group_OrderPreview_getTimeOut(skirmishAIId, groupId);
}
EXPORT(int) bridged_Group_OrderPreview_getParams(int skirmishAIId, int groupId, float* params, int params_sizeMax) {

	return id_clb[skirmishAIId]->Group_OrderPreview_getParams(skirmishAIId, groupId, params, params_sizeMax);
}
EXPORT(bool) bridged_Group_isSelected(int skirmishAIId, int groupId) {

	return id_clb[skirmishAIId]->Group_isSelected(skirmishAIId, groupId);
}
EXPORT(const char*) bridged_Mod_getFileName(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getFileName(skirmishAIId);
}
EXPORT(int) bridged_Mod_getHash(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getHash(skirmishAIId);
}
EXPORT(const char*) bridged_Mod_getHumanName(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getHumanName(skirmishAIId);
}
EXPORT(const char*) bridged_Mod_getShortName(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getShortName(skirmishAIId);
}
EXPORT(const char*) bridged_Mod_getVersion(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getVersion(skirmishAIId);
}
EXPORT(const char*) bridged_Mod_getMutator(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getMutator(skirmishAIId);
}
EXPORT(const char*) bridged_Mod_getDescription(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getDescription(skirmishAIId);
}
EXPORT(bool) bridged_Mod_getAllowTeamColors(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getAllowTeamColors(skirmishAIId);
}
EXPORT(bool) bridged_Mod_getConstructionDecay(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getConstructionDecay(skirmishAIId);
}
EXPORT(int) bridged_Mod_getConstructionDecayTime(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getConstructionDecayTime(skirmishAIId);
}
EXPORT(float) bridged_Mod_getConstructionDecaySpeed(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getConstructionDecaySpeed(skirmishAIId);
}
EXPORT(int) bridged_Mod_getMultiReclaim(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getMultiReclaim(skirmishAIId);
}
EXPORT(int) bridged_Mod_getReclaimMethod(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimMethod(skirmishAIId);
}
EXPORT(int) bridged_Mod_getReclaimUnitMethod(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimUnitMethod(skirmishAIId);
}
EXPORT(float) bridged_Mod_getReclaimUnitEnergyCostFactor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimUnitEnergyCostFactor(skirmishAIId);
}
EXPORT(float) bridged_Mod_getReclaimUnitEfficiency(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimUnitEfficiency(skirmishAIId);
}
EXPORT(float) bridged_Mod_getReclaimFeatureEnergyCostFactor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimFeatureEnergyCostFactor(skirmishAIId);
}
EXPORT(bool) bridged_Mod_getReclaimAllowEnemies(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimAllowEnemies(skirmishAIId);
}
EXPORT(bool) bridged_Mod_getReclaimAllowAllies(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getReclaimAllowAllies(skirmishAIId);
}
EXPORT(float) bridged_Mod_getRepairEnergyCostFactor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getRepairEnergyCostFactor(skirmishAIId);
}
EXPORT(float) bridged_Mod_getResurrectEnergyCostFactor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getResurrectEnergyCostFactor(skirmishAIId);
}
EXPORT(float) bridged_Mod_getCaptureEnergyCostFactor(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getCaptureEnergyCostFactor(skirmishAIId);
}
EXPORT(int) bridged_Mod_getTransportGround(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getTransportGround(skirmishAIId);
}
EXPORT(int) bridged_Mod_getTransportHover(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getTransportHover(skirmishAIId);
}
EXPORT(int) bridged_Mod_getTransportShip(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getTransportShip(skirmishAIId);
}
EXPORT(int) bridged_Mod_getTransportAir(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getTransportAir(skirmishAIId);
}
EXPORT(int) bridged_Mod_getFireAtKilled(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getFireAtKilled(skirmishAIId);
}
EXPORT(int) bridged_Mod_getFireAtCrashing(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getFireAtCrashing(skirmishAIId);
}
EXPORT(int) bridged_Mod_getFlankingBonusModeDefault(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getFlankingBonusModeDefault(skirmishAIId);
}
EXPORT(int) bridged_Mod_getLosMipLevel(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getLosMipLevel(skirmishAIId);
}
EXPORT(int) bridged_Mod_getAirMipLevel(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getAirMipLevel(skirmishAIId);
}
EXPORT(float) bridged_Mod_getLosMul(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getLosMul(skirmishAIId);
}
EXPORT(float) bridged_Mod_getAirLosMul(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getAirLosMul(skirmishAIId);
}
EXPORT(bool) bridged_Mod_getRequireSonarUnderWater(int skirmishAIId) {

	return id_clb[skirmishAIId]->Mod_getRequireSonarUnderWater(skirmishAIId);
}
EXPORT(int) bridged_Map_getChecksum(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getChecksum(skirmishAIId);
}
EXPORT(void) bridged_Map_getStartPos(int skirmishAIId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_getStartPos(skirmishAIId, return_posF3_out);
}
EXPORT(void) bridged_Map_getMousePos(int skirmishAIId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_getMousePos(skirmishAIId, return_posF3_out);
}
EXPORT(bool) bridged_Map_isPosInCamera(int skirmishAIId, float* pos_posF3, float radius) {

	return id_clb[skirmishAIId]->Map_isPosInCamera(skirmishAIId, pos_posF3, radius);
}
EXPORT(int) bridged_Map_getWidth(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getWidth(skirmishAIId);
}
EXPORT(int) bridged_Map_getHeight(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getHeight(skirmishAIId);
}
EXPORT(int) bridged_Map_getHeightMap(int skirmishAIId, float* heights, int heights_sizeMax) {

	return id_clb[skirmishAIId]->Map_getHeightMap(skirmishAIId, heights, heights_sizeMax);
}
EXPORT(int) bridged_Map_getCornersHeightMap(int skirmishAIId, float* cornerHeights, int cornerHeights_sizeMax) {

	return id_clb[skirmishAIId]->Map_getCornersHeightMap(skirmishAIId, cornerHeights, cornerHeights_sizeMax);
}
EXPORT(float) bridged_Map_getMinHeight(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getMinHeight(skirmishAIId);
}
EXPORT(float) bridged_Map_getMaxHeight(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getMaxHeight(skirmishAIId);
}
EXPORT(int) bridged_Map_getSlopeMap(int skirmishAIId, float* slopes, int slopes_sizeMax) {

	return id_clb[skirmishAIId]->Map_getSlopeMap(skirmishAIId, slopes, slopes_sizeMax);
}
EXPORT(int) bridged_Map_getLosMap(int skirmishAIId, int* losValues, int losValues_sizeMax) {

	return id_clb[skirmishAIId]->Map_getLosMap(skirmishAIId, losValues, losValues_sizeMax);
}
EXPORT(int) bridged_Map_getRadarMap(int skirmishAIId, int* radarValues, int radarValues_sizeMax) {

	return id_clb[skirmishAIId]->Map_getRadarMap(skirmishAIId, radarValues, radarValues_sizeMax);
}
EXPORT(int) bridged_Map_getJammerMap(int skirmishAIId, int* jammerValues, int jammerValues_sizeMax) {

	return id_clb[skirmishAIId]->Map_getJammerMap(skirmishAIId, jammerValues, jammerValues_sizeMax);
}
EXPORT(int) bridged_Map_getResourceMapRaw(int skirmishAIId, int resourceId, short* resources, int resources_sizeMax) {

	return id_clb[skirmishAIId]->Map_getResourceMapRaw(skirmishAIId, resourceId, resources, resources_sizeMax);
}
EXPORT(int) bridged_Map_getResourceMapSpotsPositions(int skirmishAIId, int resourceId, float* spots_AposF3, int spots_AposF3_sizeMax) {

	return id_clb[skirmishAIId]->Map_getResourceMapSpotsPositions(skirmishAIId, resourceId, spots_AposF3, spots_AposF3_sizeMax);
}
EXPORT(float) bridged_Map_getResourceMapSpotsAverageIncome(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Map_getResourceMapSpotsAverageIncome(skirmishAIId, resourceId);
}
EXPORT(void) bridged_Map_getResourceMapSpotsNearest(int skirmishAIId, int resourceId, float* pos_posF3, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_getResourceMapSpotsNearest(skirmishAIId, resourceId, pos_posF3, return_posF3_out);
}
EXPORT(int) bridged_Map_getHash(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getHash(skirmishAIId);
}
EXPORT(const char*) bridged_Map_getName(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getName(skirmishAIId);
}
EXPORT(const char*) bridged_Map_getHumanName(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getHumanName(skirmishAIId);
}
EXPORT(float) bridged_Map_getElevationAt(int skirmishAIId, float x, float z) {

	return id_clb[skirmishAIId]->Map_getElevationAt(skirmishAIId, x, z);
}
EXPORT(float) bridged_Map_getMaxResource(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Map_getMaxResource(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Map_getExtractorRadius(int skirmishAIId, int resourceId) {

	return id_clb[skirmishAIId]->Map_getExtractorRadius(skirmishAIId, resourceId);
}
EXPORT(float) bridged_Map_getMinWind(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getMinWind(skirmishAIId);
}
EXPORT(float) bridged_Map_getMaxWind(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getMaxWind(skirmishAIId);
}
EXPORT(float) bridged_Map_getCurWind(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getCurWind(skirmishAIId);
}
EXPORT(float) bridged_Map_getTidalStrength(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getTidalStrength(skirmishAIId);
}
EXPORT(float) bridged_Map_getGravity(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getGravity(skirmishAIId);
}
EXPORT(float) bridged_Map_getWaterDamage(int skirmishAIId) {

	return id_clb[skirmishAIId]->Map_getWaterDamage(skirmishAIId);
}
EXPORT(int) bridged_Map_getPoints(int skirmishAIId, bool includeAllies) {

	return id_clb[skirmishAIId]->Map_getPoints(skirmishAIId, includeAllies);
}
EXPORT(void) bridged_Map_Point_getPosition(int skirmishAIId, int pointId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_Point_getPosition(skirmishAIId, pointId, return_posF3_out);
}
EXPORT(void) bridged_Map_Point_getColor(int skirmishAIId, int pointId, short* return_colorS3_out) {

	id_clb[skirmishAIId]->Map_Point_getColor(skirmishAIId, pointId, return_colorS3_out);
}
EXPORT(const char*) bridged_Map_Point_getLabel(int skirmishAIId, int pointId) {

	return id_clb[skirmishAIId]->Map_Point_getLabel(skirmishAIId, pointId);
}
EXPORT(int) bridged_Map_getLines(int skirmishAIId, bool includeAllies) {

	return id_clb[skirmishAIId]->Map_getLines(skirmishAIId, includeAllies);
}
EXPORT(void) bridged_Map_Line_getFirstPosition(int skirmishAIId, int lineId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_Line_getFirstPosition(skirmishAIId, lineId, return_posF3_out);
}
EXPORT(void) bridged_Map_Line_getSecondPosition(int skirmishAIId, int lineId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_Line_getSecondPosition(skirmishAIId, lineId, return_posF3_out);
}
EXPORT(void) bridged_Map_Line_getColor(int skirmishAIId, int lineId, short* return_colorS3_out) {

	id_clb[skirmishAIId]->Map_Line_getColor(skirmishAIId, lineId, return_colorS3_out);
}
EXPORT(bool) bridged_Map_isPossibleToBuildAt(int skirmishAIId, int unitDefId, float* pos_posF3, int facing) {

	return id_clb[skirmishAIId]->Map_isPossibleToBuildAt(skirmishAIId, unitDefId, pos_posF3, facing);
}
EXPORT(void) bridged_Map_findClosestBuildSite(int skirmishAIId, int unitDefId, float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out) {

	id_clb[skirmishAIId]->Map_findClosestBuildSite(skirmishAIId, unitDefId, pos_posF3, searchRadius, minDist, facing, return_posF3_out);
}
EXPORT(int) bridged_getFeatureDefs(int skirmishAIId, int* featureDefIds, int featureDefIds_sizeMax) {

	return id_clb[skirmishAIId]->getFeatureDefs(skirmishAIId, featureDefIds, featureDefIds_sizeMax);
}
EXPORT(const char*) bridged_FeatureDef_getName(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getName(skirmishAIId, featureDefId);
}
EXPORT(const char*) bridged_FeatureDef_getDescription(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getDescription(skirmishAIId, featureDefId);
}
EXPORT(const char*) bridged_FeatureDef_getFileName(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getFileName(skirmishAIId, featureDefId);
}
EXPORT(float) bridged_FeatureDef_getContainedResource(int skirmishAIId, int featureDefId, int resourceId) {

	return id_clb[skirmishAIId]->FeatureDef_getContainedResource(skirmishAIId, featureDefId, resourceId);
}
EXPORT(float) bridged_FeatureDef_getMaxHealth(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getMaxHealth(skirmishAIId, featureDefId);
}
EXPORT(float) bridged_FeatureDef_getReclaimTime(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getReclaimTime(skirmishAIId, featureDefId);
}
EXPORT(float) bridged_FeatureDef_getMass(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getMass(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isUpright(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isUpright(skirmishAIId, featureDefId);
}
EXPORT(int) bridged_FeatureDef_getDrawType(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getDrawType(skirmishAIId, featureDefId);
}
EXPORT(const char*) bridged_FeatureDef_getModelName(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getModelName(skirmishAIId, featureDefId);
}
EXPORT(int) bridged_FeatureDef_getResurrectable(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getResurrectable(skirmishAIId, featureDefId);
}
EXPORT(int) bridged_FeatureDef_getSmokeTime(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getSmokeTime(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isDestructable(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isDestructable(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isReclaimable(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isReclaimable(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isBlocking(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isBlocking(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isBurnable(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isBurnable(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isFloating(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isFloating(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isNoSelect(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isNoSelect(skirmishAIId, featureDefId);
}
EXPORT(bool) bridged_FeatureDef_isGeoThermal(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_isGeoThermal(skirmishAIId, featureDefId);
}
EXPORT(const char*) bridged_FeatureDef_getDeathFeature(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getDeathFeature(skirmishAIId, featureDefId);
}
EXPORT(int) bridged_FeatureDef_getXSize(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getXSize(skirmishAIId, featureDefId);
}
EXPORT(int) bridged_FeatureDef_getZSize(int skirmishAIId, int featureDefId) {

	return id_clb[skirmishAIId]->FeatureDef_getZSize(skirmishAIId, featureDefId);
}
EXPORT(int) bridged_FeatureDef_getCustomParams(int skirmishAIId, int featureDefId, const char** keys, const char** values) {

	return id_clb[skirmishAIId]->FeatureDef_getCustomParams(skirmishAIId, featureDefId, keys, values);
}
EXPORT(int) bridged_getFeatures(int skirmishAIId, int* featureIds, int featureIds_sizeMax) {

	return id_clb[skirmishAIId]->getFeatures(skirmishAIId, featureIds, featureIds_sizeMax);
}
EXPORT(int) bridged_getFeaturesIn(int skirmishAIId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax) {

	return id_clb[skirmishAIId]->getFeaturesIn(skirmishAIId, pos_posF3, radius, featureIds, featureIds_sizeMax);
}
EXPORT(int) bridged_Feature_getDef(int skirmishAIId, int featureId) {

	return id_clb[skirmishAIId]->Feature_getDef(skirmishAIId, featureId);
}
EXPORT(float) bridged_Feature_getHealth(int skirmishAIId, int featureId) {

	return id_clb[skirmishAIId]->Feature_getHealth(skirmishAIId, featureId);
}
EXPORT(float) bridged_Feature_getReclaimLeft(int skirmishAIId, int featureId) {

	return id_clb[skirmishAIId]->Feature_getReclaimLeft(skirmishAIId, featureId);
}
EXPORT(void) bridged_Feature_getPosition(int skirmishAIId, int featureId, float* return_posF3_out) {

	id_clb[skirmishAIId]->Feature_getPosition(skirmishAIId, featureId, return_posF3_out);
}
EXPORT(int) bridged_getWeaponDefs(int skirmishAIId) {

	return id_clb[skirmishAIId]->getWeaponDefs(skirmishAIId);
}
EXPORT(int) bridged_getWeaponDefByName(int skirmishAIId, const char* weaponDefName) {

	return id_clb[skirmishAIId]->getWeaponDefByName(skirmishAIId, weaponDefName);
}
EXPORT(const char*) bridged_WeaponDef_getName(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getName(skirmishAIId, weaponDefId);
}
EXPORT(const char*) bridged_WeaponDef_getType(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getType(skirmishAIId, weaponDefId);
}
EXPORT(const char*) bridged_WeaponDef_getDescription(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getDescription(skirmishAIId, weaponDefId);
}
EXPORT(const char*) bridged_WeaponDef_getFileName(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getFileName(skirmishAIId, weaponDefId);
}
EXPORT(const char*) bridged_WeaponDef_getCegTag(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCegTag(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getRange(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getRange(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getHeightMod(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getHeightMod(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getAccuracy(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getAccuracy(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getSprayAngle(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getSprayAngle(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getMovingAccuracy(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getMovingAccuracy(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getTargetMoveError(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getTargetMoveError(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getLeadLimit(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getLeadLimit(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getLeadBonus(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getLeadBonus(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getPredictBoost(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getPredictBoost(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getNumDamageTypes(int skirmishAIId) {

	return id_clb[skirmishAIId]->WeaponDef_getNumDamageTypes(skirmishAIId);
}
EXPORT(int) bridged_WeaponDef_Damage_getParalyzeDamageTime(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Damage_getParalyzeDamageTime(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Damage_getImpulseFactor(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Damage_getImpulseFactor(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Damage_getImpulseBoost(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Damage_getImpulseBoost(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Damage_getCraterMult(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Damage_getCraterMult(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Damage_getCraterBoost(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Damage_getCraterBoost(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_Damage_getTypes(int skirmishAIId, int weaponDefId, float* types, int types_sizeMax) {

	return id_clb[skirmishAIId]->WeaponDef_Damage_getTypes(skirmishAIId, weaponDefId, types, types_sizeMax);
}
EXPORT(float) bridged_WeaponDef_getAreaOfEffect(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getAreaOfEffect(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isNoSelfDamage(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isNoSelfDamage(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getFireStarter(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getFireStarter(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getEdgeEffectiveness(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getEdgeEffectiveness(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getSize(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getSize(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getSizeGrowth(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getSizeGrowth(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getCollisionSize(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCollisionSize(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getSalvoSize(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getSalvoSize(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getSalvoDelay(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getSalvoDelay(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getReload(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getReload(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getBeamTime(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getBeamTime(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isBeamBurst(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isBeamBurst(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isWaterBounce(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isWaterBounce(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isGroundBounce(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isGroundBounce(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getBounceRebound(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getBounceRebound(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getBounceSlip(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getBounceSlip(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getNumBounce(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getNumBounce(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getMaxAngle(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getMaxAngle(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getUpTime(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getUpTime(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getFlightTime(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getFlightTime(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getCost(int skirmishAIId, int weaponDefId, int resourceId) {

	return id_clb[skirmishAIId]->WeaponDef_getCost(skirmishAIId, weaponDefId, resourceId);
}
EXPORT(int) bridged_WeaponDef_getProjectilesPerShot(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getProjectilesPerShot(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isTurret(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isTurret(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isOnlyForward(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isOnlyForward(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isFixedLauncher(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isFixedLauncher(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isWaterWeapon(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isWaterWeapon(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isFireSubmersed(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isFireSubmersed(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isSubMissile(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isSubMissile(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isTracks(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isTracks(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isDropped(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isDropped(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isParalyzer(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isParalyzer(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isImpactOnly(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isImpactOnly(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isNoAutoTarget(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isNoAutoTarget(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isManualFire(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isManualFire(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getInterceptor(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getInterceptor(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getTargetable(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getTargetable(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isStockpileable(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isStockpileable(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getCoverageRange(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCoverageRange(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getStockpileTime(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getStockpileTime(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getIntensity(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getIntensity(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getThickness(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getThickness(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getLaserFlareSize(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getLaserFlareSize(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getCoreThickness(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCoreThickness(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getDuration(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getDuration(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getLodDistance(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getLodDistance(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getFalloffRate(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getFalloffRate(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getGraphicsType(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getGraphicsType(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isSoundTrigger(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isSoundTrigger(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isSelfExplode(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isSelfExplode(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isGravityAffected(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isGravityAffected(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getHighTrajectory(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getHighTrajectory(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getMyGravity(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getMyGravity(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isNoExplode(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isNoExplode(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getStartVelocity(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getStartVelocity(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getWeaponAcceleration(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getWeaponAcceleration(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getTurnRate(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getTurnRate(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getMaxVelocity(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getMaxVelocity(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getProjectileSpeed(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getProjectileSpeed(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getExplosionSpeed(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getExplosionSpeed(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getOnlyTargetCategory(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getOnlyTargetCategory(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getWobble(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getWobble(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getDance(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getDance(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getTrajectoryHeight(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getTrajectoryHeight(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isLargeBeamLaser(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isLargeBeamLaser(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isShield(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isShield(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isShieldRepulser(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isShieldRepulser(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isSmartShield(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isSmartShield(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isExteriorShield(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isExteriorShield(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isVisibleShield(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isVisibleShield(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isVisibleShieldRepulse(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isVisibleShieldRepulse(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getVisibleShieldHitFrames(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getVisibleShieldHitFrames(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Shield_getResourceUse(int skirmishAIId, int weaponDefId, int resourceId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getResourceUse(skirmishAIId, weaponDefId, resourceId);
}
EXPORT(float) bridged_WeaponDef_Shield_getRadius(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getRadius(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Shield_getForce(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getForce(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Shield_getMaxSpeed(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getMaxSpeed(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Shield_getPower(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getPower(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Shield_getPowerRegen(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getPowerRegen(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_Shield_getPowerRegenResource(int skirmishAIId, int weaponDefId, int resourceId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getPowerRegenResource(skirmishAIId, weaponDefId, resourceId);
}
EXPORT(float) bridged_WeaponDef_Shield_getStartingPower(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getStartingPower(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_Shield_getRechargeDelay(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getRechargeDelay(skirmishAIId, weaponDefId);
}
EXPORT(void) bridged_WeaponDef_Shield_getGoodColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out) {

	id_clb[skirmishAIId]->WeaponDef_Shield_getGoodColor(skirmishAIId, weaponDefId, return_colorS3_out);
}
EXPORT(void) bridged_WeaponDef_Shield_getBadColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out) {

	id_clb[skirmishAIId]->WeaponDef_Shield_getBadColor(skirmishAIId, weaponDefId, return_colorS3_out);
}
EXPORT(short) bridged_WeaponDef_Shield_getAlpha(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getAlpha(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_Shield_getInterceptType(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_Shield_getInterceptType(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getInterceptedByShieldType(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getInterceptedByShieldType(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isAvoidFriendly(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isAvoidFriendly(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isAvoidFeature(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isAvoidFeature(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isAvoidNeutral(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isAvoidNeutral(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getTargetBorder(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getTargetBorder(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getCylinderTargetting(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCylinderTargetting(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getMinIntensity(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getMinIntensity(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getHeightBoostFactor(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getHeightBoostFactor(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getProximityPriority(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getProximityPriority(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getCollisionFlags(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCollisionFlags(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isSweepFire(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isSweepFire(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isAbleToAttackGround(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isAbleToAttackGround(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getCameraShake(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getCameraShake(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getDynDamageExp(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getDynDamageExp(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getDynDamageMin(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getDynDamageMin(skirmishAIId, weaponDefId);
}
EXPORT(float) bridged_WeaponDef_getDynDamageRange(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_getDynDamageRange(skirmishAIId, weaponDefId);
}
EXPORT(bool) bridged_WeaponDef_isDynDamageInverted(int skirmishAIId, int weaponDefId) {

	return id_clb[skirmishAIId]->WeaponDef_isDynDamageInverted(skirmishAIId, weaponDefId);
}
EXPORT(int) bridged_WeaponDef_getCustomParams(int skirmishAIId, int weaponDefId, const char** keys, const char** values) {

	return id_clb[skirmishAIId]->WeaponDef_getCustomParams(skirmishAIId, weaponDefId, keys, values);
}
EXPORT(bool) bridged_Debug_GraphDrawer_isEnabled(int skirmishAIId) {

	return id_clb[skirmishAIId]->Debug_GraphDrawer_isEnabled(skirmishAIId);
}



EXPORT(int) bridged_Cheats_setMyIncomeMultiplier(int skirmishAIId, float factor) {

	struct SSetMyIncomeMultiplierCheatCommand commandData;
	int internal_ret;
	commandData.factor = factor;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Cheats_giveMeResource(int skirmishAIId, int resourceId, float amount) {

	struct SGiveMeResourceCheatCommand commandData;
	int internal_ret;
	commandData.resourceId = resourceId;
	commandData.amount = amount;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_GIVE_ME_RESOURCE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Cheats_giveMeUnit(int skirmishAIId, int unitDefId, float* pos_posF3) {

	struct SGiveMeNewUnitCheatCommand commandData;
	int internal_ret;
	commandData.unitDefId = unitDefId;
	commandData.pos_posF3 = pos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_GIVE_ME_NEW_UNIT, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_newUnitId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Game_sendTextMessage(int skirmishAIId, const char* text, int zone) {

	struct SSendTextMessageCommand commandData;
	int internal_ret;
	commandData.text = text;
	commandData.zone = zone;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_TEXT_MESSAGE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Game_setLastMessagePosition(int skirmishAIId, float* pos_posF3) {

	struct SSetLastPosMessageCommand commandData;
	int internal_ret;
	commandData.pos_posF3 = pos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SET_LAST_POS_MESSAGE, &commandData);

	return internal_ret;
}

EXPORT(bool) bridged_Economy_sendResource(int skirmishAIId, int resourceId, float amount, int receivingTeamId) {

	struct SSendResourcesCommand commandData;
	int internal_ret;
	commandData.resourceId = resourceId;
	commandData.amount = amount;
	commandData.receivingTeamId = receivingTeamId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_RESOURCES, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_isExecuted;
	} else {
		return (bool)0;
	}
}

EXPORT(int) bridged_Economy_sendUnits(int skirmishAIId, int* unitIds, int unitIds_size, int receivingTeamId) {

	struct SSendUnitsCommand commandData;
	int internal_ret;
	commandData.unitIds = unitIds;
	commandData.unitIds_size = unitIds_size;
	commandData.receivingTeamId = receivingTeamId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_UNITS, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_sentUnits;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Group_create(int skirmishAIId) {

	struct SCreateGroupCommand commandData;
	int internal_ret;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_CREATE, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_groupId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Group_erase(int skirmishAIId, int groupId) {

	struct SEraseGroupCommand commandData;
	int internal_ret;
	commandData.groupId = groupId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ERASE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Pathing_initPath(int skirmishAIId, float* start_posF3, float* end_posF3, int pathType, float goalRadius) {

	struct SInitPathCommand commandData;
	int internal_ret;
	commandData.start_posF3 = start_posF3;
	commandData.end_posF3 = end_posF3;
	commandData.pathType = pathType;
	commandData.goalRadius = goalRadius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_INIT, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_pathId;
	} else {
		return (int)0;
	}
}

EXPORT(float) bridged_Pathing_getApproximateLength(int skirmishAIId, float* start_posF3, float* end_posF3, int pathType, float goalRadius) {

	struct SGetApproximateLengthPathCommand commandData;
	int internal_ret;
	commandData.start_posF3 = start_posF3;
	commandData.end_posF3 = end_posF3;
	commandData.pathType = pathType;
	commandData.goalRadius = goalRadius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_APPROXIMATE_LENGTH, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_approximatePathLength;
	} else {
		return (float)0;
	}
}

EXPORT(int) bridged_Pathing_getNextWaypoint(int skirmishAIId, int pathId, float* ret_nextWaypoint_posF3_out) {

	struct SGetNextWaypointPathCommand commandData;
	int internal_ret;
	commandData.pathId = pathId;
	commandData.ret_nextWaypoint_posF3_out = ret_nextWaypoint_posF3_out;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_NEXT_WAYPOINT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Pathing_freePath(int skirmishAIId, int pathId) {

	struct SFreePathCommand commandData;
	int internal_ret;
	commandData.pathId = pathId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_FREE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Lua_callRules(int skirmishAIId, const char* inData, int inSize, const char* ret_outData) {

	struct SCallLuaRulesCommand commandData;
	int internal_ret;
	commandData.inData = inData;
	commandData.inSize = inSize;
	commandData.ret_outData = ret_outData;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_RULES, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Lua_callUI(int skirmishAIId, const char* inData, int inSize, const char* ret_outData) {

	struct SCallLuaUICommand commandData;
	int internal_ret;
	commandData.inData = inData;
	commandData.inSize = inSize;
	commandData.ret_outData = ret_outData;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_UI, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Game_sendStartPosition(int skirmishAIId, bool ready, float* pos_posF3) {

	struct SSendStartPosCommand commandData;
	int internal_ret;
	commandData.ready = ready;
	commandData.pos_posF3 = pos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_START_POS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_addNotification(int skirmishAIId, float* pos_posF3, short* color_colorS3, short alpha) {

	struct SAddNotificationDrawerCommand commandData;
	int internal_ret;
	commandData.pos_posF3 = pos_posF3;
	commandData.color_colorS3 = color_colorS3;
	commandData.alpha = alpha;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_ADD_NOTIFICATION, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_addPoint(int skirmishAIId, float* pos_posF3, const char* label) {

	struct SAddPointDrawCommand commandData;
	int internal_ret;
	commandData.pos_posF3 = pos_posF3;
	commandData.label = label;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_ADD, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_deletePointsAndLines(int skirmishAIId, float* pos_posF3) {

	struct SRemovePointDrawCommand commandData;
	int internal_ret;
	commandData.pos_posF3 = pos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_REMOVE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_addLine(int skirmishAIId, float* posFrom_posF3, float* posTo_posF3) {

	struct SAddLineDrawCommand commandData;
	int internal_ret;
	commandData.posFrom_posF3 = posFrom_posF3;
	commandData.posTo_posF3 = posTo_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_LINE_ADD, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_start(int skirmishAIId, float* pos_posF3, short* color_colorS3, short alpha) {

	struct SStartPathDrawerCommand commandData;
	int internal_ret;
	commandData.pos_posF3 = pos_posF3;
	commandData.color_colorS3 = color_colorS3;
	commandData.alpha = alpha;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_START, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_finish(int skirmishAIId, bool iAmUseless) {

	struct SFinishPathDrawerCommand commandData;
	int internal_ret;
	commandData.iAmUseless = iAmUseless;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_FINISH, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_drawLine(int skirmishAIId, float* endPos_posF3, short* color_colorS3, short alpha) {

	struct SDrawLinePathDrawerCommand commandData;
	int internal_ret;
	commandData.endPos_posF3 = endPos_posF3;
	commandData.color_colorS3 = color_colorS3;
	commandData.alpha = alpha;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_drawLineAndCommandIcon(int skirmishAIId, int cmdId, float* endPos_posF3, short* color_colorS3, short alpha) {

	struct SDrawLineAndIconPathDrawerCommand commandData;
	int internal_ret;
	commandData.cmdId = cmdId;
	commandData.endPos_posF3 = endPos_posF3;
	commandData.color_colorS3 = color_colorS3;
	commandData.alpha = alpha;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_drawIcon(int skirmishAIId, int cmdId) {

	struct SDrawIconAtLastPosPathDrawerCommand commandData;
	int internal_ret;
	commandData.cmdId = cmdId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_suspend(int skirmishAIId, float* endPos_posF3, short* color_colorS3, short alpha) {

	struct SBreakPathDrawerCommand commandData;
	int internal_ret;
	commandData.endPos_posF3 = endPos_posF3;
	commandData.color_colorS3 = color_colorS3;
	commandData.alpha = alpha;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_BREAK, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_PathDrawer_restart(int skirmishAIId, bool sameColor) {

	struct SRestartPathDrawerCommand commandData;
	int internal_ret;
	commandData.sameColor = sameColor;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_Figure_drawSpline(int skirmishAIId, float* pos1_posF3, float* pos2_posF3, float* pos3_posF3, float* pos4_posF3, float width, bool arrow, int lifeTime, int figureGroupId) {

	struct SCreateSplineFigureDrawerCommand commandData;
	int internal_ret;
	commandData.pos1_posF3 = pos1_posF3;
	commandData.pos2_posF3 = pos2_posF3;
	commandData.pos3_posF3 = pos3_posF3;
	commandData.pos4_posF3 = pos4_posF3;
	commandData.width = width;
	commandData.arrow = arrow;
	commandData.lifeTime = lifeTime;
	commandData.figureGroupId = figureGroupId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_SPLINE, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_newFigureGroupId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Map_Drawer_Figure_drawLine(int skirmishAIId, float* pos1_posF3, float* pos2_posF3, float width, bool arrow, int lifeTime, int figureGroupId) {

	struct SCreateLineFigureDrawerCommand commandData;
	int internal_ret;
	commandData.pos1_posF3 = pos1_posF3;
	commandData.pos2_posF3 = pos2_posF3;
	commandData.width = width;
	commandData.arrow = arrow;
	commandData.lifeTime = lifeTime;
	commandData.figureGroupId = figureGroupId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_LINE, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_newFigureGroupId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Map_Drawer_Figure_setColor(int skirmishAIId, int figureGroupId, short* color_colorS3, short alpha) {

	struct SSetColorFigureDrawerCommand commandData;
	int internal_ret;
	commandData.figureGroupId = figureGroupId;
	commandData.color_colorS3 = color_colorS3;
	commandData.alpha = alpha;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_SET_COLOR, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_Figure_remove(int skirmishAIId, int figureGroupId) {

	struct SDeleteFigureDrawerCommand commandData;
	int internal_ret;
	commandData.figureGroupId = figureGroupId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_DELETE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Map_Drawer_drawUnit(int skirmishAIId, int toDrawUnitDefId, float* pos_posF3, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing) {

	struct SDrawUnitDrawerCommand commandData;
	int internal_ret;
	commandData.toDrawUnitDefId = toDrawUnitDefId;
	commandData.pos_posF3 = pos_posF3;
	commandData.rotation = rotation;
	commandData.lifeTime = lifeTime;
	commandData.teamId = teamId;
	commandData.transparent = transparent;
	commandData.drawBorder = drawBorder;
	commandData.facing = facing;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_DRAW_UNIT, &commandData);

	return internal_ret;
}

static int internal_bridged_Unit_build(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toBuildUnitDefId, float* buildPos_posF3, int facing) {

	struct SBuildUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toBuildUnitDefId = toBuildUnitDefId;
	commandData.buildPos_posF3 = buildPos_posF3;
	commandData.facing = facing;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_BUILD, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_build(int skirmishAIId, int unitId, int toBuildUnitDefId, float* buildPos_posF3, int facing, short options, int timeOut) { // REF:toBuildUnitDefId->UnitDef error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_build(skirmishAIId, unitId, groupId, options, timeOut, toBuildUnitDefId, buildPos_posF3, facing);
}

EXPORT(int) bridged_Group_build(int skirmishAIId, int groupId, int toBuildUnitDefId, float* buildPos_posF3, int facing, short options, int timeOut) { // REF:toBuildUnitDefId->UnitDef error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_build(skirmishAIId, unitId, groupId, options, timeOut, toBuildUnitDefId, buildPos_posF3, facing);
}

static int internal_bridged_Unit_stop(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SStopUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_STOP, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_stop(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_stop(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_stop(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_stop(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_wait(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SWaitUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_wait(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_wait(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_wait(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_wait(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_waitFor(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int time) {

	struct STimeWaitUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.time = time;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_TIME, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_waitFor(int skirmishAIId, int unitId, int time, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_waitFor(skirmishAIId, unitId, groupId, options, timeOut, time);
}

EXPORT(int) bridged_Group_waitFor(int skirmishAIId, int groupId, int time, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_waitFor(skirmishAIId, unitId, groupId, options, timeOut, time);
}

static int internal_bridged_Unit_waitForDeathOf(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toDieUnitId) {

	struct SDeathWaitUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toDieUnitId = toDieUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_DEATH, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_waitForDeathOf(int skirmishAIId, int unitId, int toDieUnitId, short options, int timeOut) { // REF:toDieUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_waitForDeathOf(skirmishAIId, unitId, groupId, options, timeOut, toDieUnitId);
}

EXPORT(int) bridged_Group_waitForDeathOf(int skirmishAIId, int groupId, int toDieUnitId, short options, int timeOut) { // REF:toDieUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_waitForDeathOf(skirmishAIId, unitId, groupId, options, timeOut, toDieUnitId);
}

static int internal_bridged_Unit_waitForSquadSize(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int numUnits) {

	struct SSquadWaitUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.numUnits = numUnits;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_SQUAD, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_waitForSquadSize(int skirmishAIId, int unitId, int numUnits, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_waitForSquadSize(skirmishAIId, unitId, groupId, options, timeOut, numUnits);
}

EXPORT(int) bridged_Group_waitForSquadSize(int skirmishAIId, int groupId, int numUnits, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_waitForSquadSize(skirmishAIId, unitId, groupId, options, timeOut, numUnits);
}

static int internal_bridged_Unit_waitForAll(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SGatherWaitUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_GATHER, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_waitForAll(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_waitForAll(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_waitForAll(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_waitForAll(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_moveTo(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* toPos_posF3) {

	struct SMoveUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toPos_posF3 = toPos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_MOVE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_moveTo(int skirmishAIId, int unitId, float* toPos_posF3, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_moveTo(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3);
}

EXPORT(int) bridged_Group_moveTo(int skirmishAIId, int groupId, float* toPos_posF3, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_moveTo(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3);
}

static int internal_bridged_Unit_patrolTo(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* toPos_posF3) {

	struct SPatrolUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toPos_posF3 = toPos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_PATROL, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_patrolTo(int skirmishAIId, int unitId, float* toPos_posF3, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_patrolTo(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3);
}

EXPORT(int) bridged_Group_patrolTo(int skirmishAIId, int groupId, float* toPos_posF3, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_patrolTo(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3);
}

static int internal_bridged_Unit_fight(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* toPos_posF3) {

	struct SFightUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toPos_posF3 = toPos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_FIGHT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_fight(int skirmishAIId, int unitId, float* toPos_posF3, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_fight(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3);
}

EXPORT(int) bridged_Group_fight(int skirmishAIId, int groupId, float* toPos_posF3, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_fight(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3);
}

static int internal_bridged_Unit_attack(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toAttackUnitId) {

	struct SAttackUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toAttackUnitId = toAttackUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_ATTACK, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_attack(int skirmishAIId, int unitId, int toAttackUnitId, short options, int timeOut) { // REF:toAttackUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_attack(skirmishAIId, unitId, groupId, options, timeOut, toAttackUnitId);
}

EXPORT(int) bridged_Group_attack(int skirmishAIId, int groupId, int toAttackUnitId, short options, int timeOut) { // REF:toAttackUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_attack(skirmishAIId, unitId, groupId, options, timeOut, toAttackUnitId);
}

static int internal_bridged_Unit_attackArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* toAttackPos_posF3, float radius) {

	struct SAttackAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toAttackPos_posF3 = toAttackPos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_ATTACK_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_attackArea(int skirmishAIId, int unitId, float* toAttackPos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_attackArea(skirmishAIId, unitId, groupId, options, timeOut, toAttackPos_posF3, radius);
}

EXPORT(int) bridged_Group_attackArea(int skirmishAIId, int groupId, float* toAttackPos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_attackArea(skirmishAIId, unitId, groupId, options, timeOut, toAttackPos_posF3, radius);
}

static int internal_bridged_Unit_guard(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toGuardUnitId) {

	struct SGuardUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toGuardUnitId = toGuardUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GUARD, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_guard(int skirmishAIId, int unitId, int toGuardUnitId, short options, int timeOut) { // REF:toGuardUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_guard(skirmishAIId, unitId, groupId, options, timeOut, toGuardUnitId);
}

EXPORT(int) bridged_Group_guard(int skirmishAIId, int groupId, int toGuardUnitId, short options, int timeOut) { // REF:toGuardUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_guard(skirmishAIId, unitId, groupId, options, timeOut, toGuardUnitId);
}

static int internal_bridged_Unit_aiSelect(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SAiSelectUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_AI_SELECT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_aiSelect(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_aiSelect(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_aiSelect(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_aiSelect(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_addToGroup(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toGroupId) {

	struct SGroupAddUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toGroupId = toGroupId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_ADD, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_addToGroup(int skirmishAIId, int unitId, int toGroupId, short options, int timeOut) { // REF:toGroupId->Group error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_addToGroup(skirmishAIId, unitId, groupId, options, timeOut, toGroupId);
}

EXPORT(int) bridged_Group_addToGroup(int skirmishAIId, int groupId, int toGroupId, short options, int timeOut) { // REF:toGroupId->Group error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_addToGroup(skirmishAIId, unitId, groupId, options, timeOut, toGroupId);
}

static int internal_bridged_Unit_removeFromGroup(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SGroupClearUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_CLEAR, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_removeFromGroup(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_removeFromGroup(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_removeFromGroup(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_removeFromGroup(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_repair(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toRepairUnitId) {

	struct SRepairUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toRepairUnitId = toRepairUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_REPAIR, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_repair(int skirmishAIId, int unitId, int toRepairUnitId, short options, int timeOut) { // REF:toRepairUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_repair(skirmishAIId, unitId, groupId, options, timeOut, toRepairUnitId);
}

EXPORT(int) bridged_Group_repair(int skirmishAIId, int groupId, int toRepairUnitId, short options, int timeOut) { // REF:toRepairUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_repair(skirmishAIId, unitId, groupId, options, timeOut, toRepairUnitId);
}

static int internal_bridged_Unit_setFireState(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int fireState) {

	struct SSetFireStateUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.fireState = fireState;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_FIRE_STATE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setFireState(int skirmishAIId, int unitId, int fireState, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setFireState(skirmishAIId, unitId, groupId, options, timeOut, fireState);
}

EXPORT(int) bridged_Group_setFireState(int skirmishAIId, int groupId, int fireState, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setFireState(skirmishAIId, unitId, groupId, options, timeOut, fireState);
}

static int internal_bridged_Unit_setMoveState(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int moveState) {

	struct SSetMoveStateUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.moveState = moveState;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_MOVE_STATE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setMoveState(int skirmishAIId, int unitId, int moveState, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setMoveState(skirmishAIId, unitId, groupId, options, timeOut, moveState);
}

EXPORT(int) bridged_Group_setMoveState(int skirmishAIId, int groupId, int moveState, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setMoveState(skirmishAIId, unitId, groupId, options, timeOut, moveState);
}

static int internal_bridged_Unit_setBase(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* basePos_posF3) {

	struct SSetBaseUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.basePos_posF3 = basePos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_BASE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setBase(int skirmishAIId, int unitId, float* basePos_posF3, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setBase(skirmishAIId, unitId, groupId, options, timeOut, basePos_posF3);
}

EXPORT(int) bridged_Group_setBase(int skirmishAIId, int groupId, float* basePos_posF3, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setBase(skirmishAIId, unitId, groupId, options, timeOut, basePos_posF3);
}

static int internal_bridged_Unit_selfDestruct(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SSelfDestroyUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SELF_DESTROY, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_selfDestruct(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_selfDestruct(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_selfDestruct(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_selfDestruct(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_setWantedMaxSpeed(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float wantedMaxSpeed) {

	struct SSetWantedMaxSpeedUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.wantedMaxSpeed = wantedMaxSpeed;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_WANTED_MAX_SPEED, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setWantedMaxSpeed(int skirmishAIId, int unitId, float wantedMaxSpeed, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setWantedMaxSpeed(skirmishAIId, unitId, groupId, options, timeOut, wantedMaxSpeed);
}

EXPORT(int) bridged_Group_setWantedMaxSpeed(int skirmishAIId, int groupId, float wantedMaxSpeed, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setWantedMaxSpeed(skirmishAIId, unitId, groupId, options, timeOut, wantedMaxSpeed);
}

static int internal_bridged_Unit_loadUnits(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int* toLoadUnitIds, int toLoadUnitIds_size) {

	struct SLoadUnitsUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toLoadUnitIds = toLoadUnitIds;
	commandData.toLoadUnitIds_size = toLoadUnitIds_size;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_LOAD_UNITS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_loadUnits(int skirmishAIId, int unitId, int* toLoadUnitIds, int toLoadUnitIds_size, short options, int timeOut) { // REF:MULTI:toLoadUnitIds->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_loadUnits(skirmishAIId, unitId, groupId, options, timeOut, toLoadUnitIds, toLoadUnitIds_size);
}

EXPORT(int) bridged_Group_loadUnits(int skirmishAIId, int groupId, int* toLoadUnitIds, int toLoadUnitIds_size, short options, int timeOut) { // REF:MULTI:toLoadUnitIds->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_loadUnits(skirmishAIId, unitId, groupId, options, timeOut, toLoadUnitIds, toLoadUnitIds_size);
}

static int internal_bridged_Unit_loadUnitsInArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* pos_posF3, float radius) {

	struct SLoadUnitsAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.pos_posF3 = pos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_LOAD_UNITS_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_loadUnitsInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_loadUnitsInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

EXPORT(int) bridged_Group_loadUnitsInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_loadUnitsInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

static int internal_bridged_Unit_loadOnto(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int transporterUnitId) {

	struct SLoadOntoUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.transporterUnitId = transporterUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_LOAD_ONTO, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_loadOnto(int skirmishAIId, int unitId, int transporterUnitId, short options, int timeOut) { // REF:transporterUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_loadOnto(skirmishAIId, unitId, groupId, options, timeOut, transporterUnitId);
}

EXPORT(int) bridged_Group_loadOnto(int skirmishAIId, int groupId, int transporterUnitId, short options, int timeOut) { // REF:transporterUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_loadOnto(skirmishAIId, unitId, groupId, options, timeOut, transporterUnitId);
}

static int internal_bridged_Unit_unload(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* toPos_posF3, int toUnloadUnitId) {

	struct SUnloadUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toPos_posF3 = toPos_posF3;
	commandData.toUnloadUnitId = toUnloadUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_UNLOAD_UNIT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_unload(int skirmishAIId, int unitId, float* toPos_posF3, int toUnloadUnitId, short options, int timeOut) { // REF:toUnloadUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_unload(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3, toUnloadUnitId);
}

EXPORT(int) bridged_Group_unload(int skirmishAIId, int groupId, float* toPos_posF3, int toUnloadUnitId, short options, int timeOut) { // REF:toUnloadUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_unload(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3, toUnloadUnitId);
}

static int internal_bridged_Unit_unloadUnitsInArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* toPos_posF3, float radius) {

	struct SUnloadUnitsAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toPos_posF3 = toPos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_UNLOAD_UNITS_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_unloadUnitsInArea(int skirmishAIId, int unitId, float* toPos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_unloadUnitsInArea(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3, radius);
}

EXPORT(int) bridged_Group_unloadUnitsInArea(int skirmishAIId, int groupId, float* toPos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_unloadUnitsInArea(skirmishAIId, unitId, groupId, options, timeOut, toPos_posF3, radius);
}

static int internal_bridged_Unit_setOn(int skirmishAIId, int unitId, int groupId, short options, int timeOut, bool on) {

	struct SSetOnOffUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.on = on;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_ON_OFF, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setOn(int skirmishAIId, int unitId, bool on, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setOn(skirmishAIId, unitId, groupId, options, timeOut, on);
}

EXPORT(int) bridged_Group_setOn(int skirmishAIId, int groupId, bool on, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setOn(skirmishAIId, unitId, groupId, options, timeOut, on);
}

static int internal_bridged_Unit_reclaimUnit(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toReclaimUnitId) {

	struct SReclaimUnitUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toReclaimUnitId = toReclaimUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RECLAIM_UNIT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_reclaimUnit(int skirmishAIId, int unitId, int toReclaimUnitId, short options, int timeOut) { // REF:toReclaimUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_reclaimUnit(skirmishAIId, unitId, groupId, options, timeOut, toReclaimUnitId);
}

EXPORT(int) bridged_Group_reclaimUnit(int skirmishAIId, int groupId, int toReclaimUnitId, short options, int timeOut) { // REF:toReclaimUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_reclaimUnit(skirmishAIId, unitId, groupId, options, timeOut, toReclaimUnitId);
}

static int internal_bridged_Unit_reclaimFeature(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toReclaimFeatureId) {

	struct SReclaimFeatureUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toReclaimFeatureId = toReclaimFeatureId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RECLAIM_FEATURE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_reclaimFeature(int skirmishAIId, int unitId, int toReclaimFeatureId, short options, int timeOut) { // REF:toReclaimFeatureId->Feature error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_reclaimFeature(skirmishAIId, unitId, groupId, options, timeOut, toReclaimFeatureId);
}

EXPORT(int) bridged_Group_reclaimFeature(int skirmishAIId, int groupId, int toReclaimFeatureId, short options, int timeOut) { // REF:toReclaimFeatureId->Feature error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_reclaimFeature(skirmishAIId, unitId, groupId, options, timeOut, toReclaimFeatureId);
}

static int internal_bridged_Unit_reclaimInArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* pos_posF3, float radius) {

	struct SReclaimAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.pos_posF3 = pos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RECLAIM_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_reclaimInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_reclaimInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

EXPORT(int) bridged_Group_reclaimInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_reclaimInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

static int internal_bridged_Unit_cloak(int skirmishAIId, int unitId, int groupId, short options, int timeOut, bool cloak) {

	struct SCloakUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.cloak = cloak;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CLOAK, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_cloak(int skirmishAIId, int unitId, bool cloak, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_cloak(skirmishAIId, unitId, groupId, options, timeOut, cloak);
}

EXPORT(int) bridged_Group_cloak(int skirmishAIId, int groupId, bool cloak, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_cloak(skirmishAIId, unitId, groupId, options, timeOut, cloak);
}

static int internal_bridged_Unit_stockpile(int skirmishAIId, int unitId, int groupId, short options, int timeOut) {

	struct SStockpileUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_STOCKPILE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_stockpile(int skirmishAIId, int unitId, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_stockpile(skirmishAIId, unitId, groupId, options, timeOut);
}

EXPORT(int) bridged_Group_stockpile(int skirmishAIId, int groupId, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_stockpile(skirmishAIId, unitId, groupId, options, timeOut);
}

static int internal_bridged_Unit_dGun(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toAttackUnitId) {

	struct SDGunUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toAttackUnitId = toAttackUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_D_GUN, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_dGun(int skirmishAIId, int unitId, int toAttackUnitId, short options, int timeOut) { // REF:toAttackUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_dGun(skirmishAIId, unitId, groupId, options, timeOut, toAttackUnitId);
}

EXPORT(int) bridged_Group_dGun(int skirmishAIId, int groupId, int toAttackUnitId, short options, int timeOut) { // REF:toAttackUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_dGun(skirmishAIId, unitId, groupId, options, timeOut, toAttackUnitId);
}

static int internal_bridged_Unit_dGunPosition(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* pos_posF3) {

	struct SDGunPosUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.pos_posF3 = pos_posF3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_D_GUN_POS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_dGunPosition(int skirmishAIId, int unitId, float* pos_posF3, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_dGunPosition(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3);
}

EXPORT(int) bridged_Group_dGunPosition(int skirmishAIId, int groupId, float* pos_posF3, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_dGunPosition(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3);
}

static int internal_bridged_Unit_restoreArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* pos_posF3, float radius) {

	struct SRestoreAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.pos_posF3 = pos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RESTORE_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_restoreArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_restoreArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

EXPORT(int) bridged_Group_restoreArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_restoreArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

static int internal_bridged_Unit_setRepeat(int skirmishAIId, int unitId, int groupId, short options, int timeOut, bool repeat) {

	struct SSetRepeatUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.repeat = repeat;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_REPEAT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setRepeat(int skirmishAIId, int unitId, bool repeat, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setRepeat(skirmishAIId, unitId, groupId, options, timeOut, repeat);
}

EXPORT(int) bridged_Group_setRepeat(int skirmishAIId, int groupId, bool repeat, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setRepeat(skirmishAIId, unitId, groupId, options, timeOut, repeat);
}

static int internal_bridged_Unit_setTrajectory(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int trajectory) {

	struct SSetTrajectoryUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.trajectory = trajectory;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_TRAJECTORY, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setTrajectory(int skirmishAIId, int unitId, int trajectory, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setTrajectory(skirmishAIId, unitId, groupId, options, timeOut, trajectory);
}

EXPORT(int) bridged_Group_setTrajectory(int skirmishAIId, int groupId, int trajectory, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setTrajectory(skirmishAIId, unitId, groupId, options, timeOut, trajectory);
}

static int internal_bridged_Unit_resurrect(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toResurrectFeatureId) {

	struct SResurrectUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toResurrectFeatureId = toResurrectFeatureId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RESURRECT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_resurrect(int skirmishAIId, int unitId, int toResurrectFeatureId, short options, int timeOut) { // REF:toResurrectFeatureId->Feature error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_resurrect(skirmishAIId, unitId, groupId, options, timeOut, toResurrectFeatureId);
}

EXPORT(int) bridged_Group_resurrect(int skirmishAIId, int groupId, int toResurrectFeatureId, short options, int timeOut) { // REF:toResurrectFeatureId->Feature error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_resurrect(skirmishAIId, unitId, groupId, options, timeOut, toResurrectFeatureId);
}

static int internal_bridged_Unit_resurrectInArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* pos_posF3, float radius) {

	struct SResurrectAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.pos_posF3 = pos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RESURRECT_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_resurrectInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_resurrectInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

EXPORT(int) bridged_Group_resurrectInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_resurrectInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

static int internal_bridged_Unit_capture(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int toCaptureUnitId) {

	struct SCaptureUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.toCaptureUnitId = toCaptureUnitId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CAPTURE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_capture(int skirmishAIId, int unitId, int toCaptureUnitId, short options, int timeOut) { // REF:toCaptureUnitId->Unit error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_capture(skirmishAIId, unitId, groupId, options, timeOut, toCaptureUnitId);
}

EXPORT(int) bridged_Group_capture(int skirmishAIId, int groupId, int toCaptureUnitId, short options, int timeOut) { // REF:toCaptureUnitId->Unit error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_capture(skirmishAIId, unitId, groupId, options, timeOut, toCaptureUnitId);
}

static int internal_bridged_Unit_captureInArea(int skirmishAIId, int unitId, int groupId, short options, int timeOut, float* pos_posF3, float radius) {

	struct SCaptureAreaUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.pos_posF3 = pos_posF3;
	commandData.radius = radius;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CAPTURE_AREA, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_captureInArea(int skirmishAIId, int unitId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_captureInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

EXPORT(int) bridged_Group_captureInArea(int skirmishAIId, int groupId, float* pos_posF3, float radius, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_captureInArea(skirmishAIId, unitId, groupId, options, timeOut, pos_posF3, radius);
}

static int internal_bridged_Unit_setAutoRepairLevel(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int autoRepairLevel) {

	struct SSetAutoRepairLevelUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.autoRepairLevel = autoRepairLevel;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setAutoRepairLevel(int skirmishAIId, int unitId, int autoRepairLevel, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setAutoRepairLevel(skirmishAIId, unitId, groupId, options, timeOut, autoRepairLevel);
}

EXPORT(int) bridged_Group_setAutoRepairLevel(int skirmishAIId, int groupId, int autoRepairLevel, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setAutoRepairLevel(skirmishAIId, unitId, groupId, options, timeOut, autoRepairLevel);
}

static int internal_bridged_Unit_setIdleMode(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int idleMode) {

	struct SSetIdleModeUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.idleMode = idleMode;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_IDLE_MODE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_setIdleMode(int skirmishAIId, int unitId, int idleMode, short options, int timeOut) { // error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_setIdleMode(skirmishAIId, unitId, groupId, options, timeOut, idleMode);
}

EXPORT(int) bridged_Group_setIdleMode(int skirmishAIId, int groupId, int idleMode, short options, int timeOut) { // error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_setIdleMode(skirmishAIId, unitId, groupId, options, timeOut, idleMode);
}

static int internal_bridged_Unit_executeCustomCommand(int skirmishAIId, int unitId, int groupId, short options, int timeOut, int cmdId, float* params, int params_size) {

	struct SCustomUnitCommand commandData;
	int internal_ret;
	commandData.unitId = unitId;
	commandData.groupId = groupId;
	commandData.options = options;
	commandData.timeOut = timeOut;
	commandData.cmdId = cmdId;
	commandData.params = params;
	commandData.params_size = params_size;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CUSTOM, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Unit_executeCustomCommand(int skirmishAIId, int unitId, int cmdId, float* params, int params_size, short options, int timeOut) { // ARRAY:params error-return:0=OK

	const int groupId = -1;
	return internal_bridged_Unit_executeCustomCommand(skirmishAIId, unitId, groupId, options, timeOut, cmdId, params, params_size);
}

EXPORT(int) bridged_Group_executeCustomCommand(int skirmishAIId, int groupId, int cmdId, float* params, int params_size, short options, int timeOut) { // ARRAY:params error-return:0=OK

	const int unitId = -1;
	return internal_bridged_Unit_executeCustomCommand(skirmishAIId, unitId, groupId, options, timeOut, cmdId, params, params_size);
}

EXPORT(int) bridged_Map_Drawer_traceRay(int skirmishAIId, float* rayPos_posF3, float* rayDir_posF3, float rayLen, int srcUnitId, int flags) {

	struct STraceRayCommand commandData;
	int internal_ret;
	commandData.rayPos_posF3 = rayPos_posF3;
	commandData.rayDir_posF3 = rayDir_posF3;
	commandData.rayLen = rayLen;
	commandData.srcUnitId = srcUnitId;
	commandData.flags = flags;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_TRACE_RAY, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_hitUnitId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Map_Drawer_traceRayFeature(int skirmishAIId, float* rayPos_posF3, float* rayDir_posF3, float rayLen, int srcUnitId, int flags) {

	struct SFeatureTraceRayCommand commandData;
	int internal_ret;
	commandData.rayPos_posF3 = rayPos_posF3;
	commandData.rayDir_posF3 = rayDir_posF3;
	commandData.rayLen = rayLen;
	commandData.srcUnitId = srcUnitId;
	commandData.flags = flags;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_TRACE_RAY_FEATURE, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_hitFeatureId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Game_setPause(int skirmishAIId, bool enable, const char* reason) {

	struct SPauseCommand commandData;
	int internal_ret;
	commandData.enable = enable;
	commandData.reason = reason;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PAUSE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_GraphDrawer_setPosition(int skirmishAIId, float x, float y) {

	struct SSetPositionGraphDrawerDebugCommand commandData;
	int internal_ret;
	commandData.x = x;
	commandData.y = y;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_SET_POS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_GraphDrawer_setSize(int skirmishAIId, float w, float h) {

	struct SSetSizeGraphDrawerDebugCommand commandData;
	int internal_ret;
	commandData.w = w;
	commandData.h = h;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_addPoint(int skirmishAIId, int lineId, float x, float y) {

	struct SAddPointLineGraphDrawerDebugCommand commandData;
	int internal_ret;
	commandData.lineId = lineId;
	commandData.x = x;
	commandData.y = y;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_deletePoints(int skirmishAIId, int lineId, int numPoints) {

	struct SDeletePointsLineGraphDrawerDebugCommand commandData;
	int internal_ret;
	commandData.lineId = lineId;
	commandData.numPoints = numPoints;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_setColor(int skirmishAIId, int lineId, short* color_colorS3) {

	struct SSetColorLineGraphDrawerDebugCommand commandData;
	int internal_ret;
	commandData.lineId = lineId;
	commandData.color_colorS3 = color_colorS3;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_GraphDrawer_GraphLine_setLabel(int skirmishAIId, int lineId, const char* label) {

	struct SSetLabelLineGraphDrawerDebugCommand commandData;
	int internal_ret;
	commandData.lineId = lineId;
	commandData.label = label;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_addOverlayTexture(int skirmishAIId, const float* texData, int w, int h) {

	struct SAddOverlayTextureDrawerDebugCommand commandData;
	int internal_ret;
	commandData.texData = texData;
	commandData.w = w;
	commandData.h = h;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD, &commandData);

	if (internal_ret == 0) {
		return commandData.ret_overlayTextureId;
	} else {
		return (int)0;
	}
}

EXPORT(int) bridged_Debug_OverlayTexture_update(int skirmishAIId, int overlayTextureId, const float* texData, int x, int y, int w, int h) {

	struct SUpdateOverlayTextureDrawerDebugCommand commandData;
	int internal_ret;
	commandData.overlayTextureId = overlayTextureId;
	commandData.texData = texData;
	commandData.x = x;
	commandData.y = y;
	commandData.w = w;
	commandData.h = h;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_OverlayTexture_remove(int skirmishAIId, int overlayTextureId) {

	struct SDeleteOverlayTextureDrawerDebugCommand commandData;
	int internal_ret;
	commandData.overlayTextureId = overlayTextureId;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_OverlayTexture_setPosition(int skirmishAIId, int overlayTextureId, float x, float y) {

	struct SSetPositionOverlayTextureDrawerDebugCommand commandData;
	int internal_ret;
	commandData.overlayTextureId = overlayTextureId;
	commandData.x = x;
	commandData.y = y;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_OverlayTexture_setSize(int skirmishAIId, int overlayTextureId, float w, float h) {

	struct SSetSizeOverlayTextureDrawerDebugCommand commandData;
	int internal_ret;
	commandData.overlayTextureId = overlayTextureId;
	commandData.w = w;
	commandData.h = h;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE, &commandData);

	return internal_ret;
}

EXPORT(int) bridged_Debug_OverlayTexture_setLabel(int skirmishAIId, int overlayTextureId, const char* label) {

	struct SSetLabelOverlayTextureDrawerDebugCommand commandData;
	int internal_ret;
	commandData.overlayTextureId = overlayTextureId;
	commandData.label = label;

	internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL, &commandData);

	return internal_ret;
}

