/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGame.h"

#include "IncludesSources.h"

	springai::WrappGame::WrappGame(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappGame::~WrappGame() {

	}

	int springai::WrappGame::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappGame::Game* springai::WrappGame::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Game* internal_ret = NULL;
		internal_ret = new springai::WrappGame(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappGame::GetCurrentFrame() {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getCurrentFrame(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappGame::GetAiInterfaceVersion() {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getAiInterfaceVersion(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappGame::GetMyTeam() {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getMyTeam(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappGame::GetMyAllyTeam() {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getMyAllyTeam(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappGame::GetPlayerTeam(int playerId) {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getPlayerTeam(this->GetSkirmishAIId(), playerId);
		return internal_ret_int;
	}

	int springai::WrappGame::GetTeams() {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getTeams(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappGame::GetTeamSide(int otherTeamId) {

		const char* internal_ret_int;

		internal_ret_int = bridged_Game_getTeamSide(this->GetSkirmishAIId(), otherTeamId);
		return internal_ret_int;
	}

	springai::AIColor springai::WrappGame::GetTeamColor(int otherTeamId) {

		short return_colorS3_out[3];

		bridged_Game_getTeamColor(this->GetSkirmishAIId(), otherTeamId, return_colorS3_out);
		springai::AIColor internal_ret((unsigned char) return_colorS3_out[0], (unsigned char) return_colorS3_out[1], (unsigned char) return_colorS3_out[2]);

		return internal_ret;
	}

	float springai::WrappGame::GetTeamIncomeMultiplier(int otherTeamId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamIncomeMultiplier(this->GetSkirmishAIId(), otherTeamId);
		return internal_ret_int;
	}

	int springai::WrappGame::GetTeamAllyTeam(int otherTeamId) {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getTeamAllyTeam(this->GetSkirmishAIId(), otherTeamId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceCurrent(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceCurrent(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceIncome(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceIncome(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceUsage(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceUsage(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceStorage(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceStorage(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourcePull(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourcePull(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceShare(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceShare(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceSent(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceSent(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceReceived(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceReceived(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	float springai::WrappGame::GetTeamResourceExcess(int otherTeamId, int resourceId) {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getTeamResourceExcess(this->GetSkirmishAIId(), otherTeamId, resourceId);
		return internal_ret_int;
	}

	bool springai::WrappGame::IsAllied(int firstAllyTeamId, int secondAllyTeamId) {

		bool internal_ret_int;

		internal_ret_int = bridged_Game_isAllied(this->GetSkirmishAIId(), firstAllyTeamId, secondAllyTeamId);
		return internal_ret_int;
	}

	bool springai::WrappGame::IsExceptionHandlingEnabled() {

		bool internal_ret_int;

		internal_ret_int = bridged_Game_isExceptionHandlingEnabled(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappGame::IsDebugModeEnabled() {

		bool internal_ret_int;

		internal_ret_int = bridged_Game_isDebugModeEnabled(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappGame::GetMode() {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getMode(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappGame::IsPaused() {

		bool internal_ret_int;

		internal_ret_int = bridged_Game_isPaused(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappGame::GetSpeedFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_Game_getSpeedFactor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappGame::GetSetupScript() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Game_getSetupScript(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappGame::GetCategoryFlag(const char* categoryName) {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getCategoryFlag(this->GetSkirmishAIId(), categoryName);
		return internal_ret_int;
	}

	int springai::WrappGame::GetCategoriesFlag(const char* categoryNames) {

		int internal_ret_int;

		internal_ret_int = bridged_Game_getCategoriesFlag(this->GetSkirmishAIId(), categoryNames);
		return internal_ret_int;
	}

	void springai::WrappGame::GetCategoryName(int categoryFlag, char* name, int name_sizeMax) {

		bridged_Game_getCategoryName(this->GetSkirmishAIId(), categoryFlag, name, name_sizeMax);
	}

	std::vector<springai::GameRulesParam*> springai::WrappGame::GetGameRulesParams() {

		int paramIds_sizeMax;
		int paramIds_raw_size;
		int* paramIds;
		std::vector<springai::GameRulesParam*> paramIds_list;
		int paramIds_size;
		int internal_ret_int;

		paramIds_sizeMax = INT_MAX;
		paramIds = NULL;
		paramIds_size = bridged_Game_getGameRulesParams(this->GetSkirmishAIId(), paramIds, paramIds_sizeMax);
		paramIds_sizeMax = paramIds_size;
		paramIds_raw_size = paramIds_size;
		paramIds = new int[paramIds_raw_size];

		internal_ret_int = bridged_Game_getGameRulesParams(this->GetSkirmishAIId(), paramIds, paramIds_sizeMax);
		paramIds_list.reserve(paramIds_size);
		for (int i=0; i < paramIds_sizeMax; ++i) {
			paramIds_list.push_back(springai::WrappGameRulesParam::GetInstance(skirmishAIId, paramIds[i]));
		}
		delete[] paramIds;

		return paramIds_list;
	}

	springai::GameRulesParam* springai::WrappGame::GetGameRulesParamByName(const char* rulesParamName) {

		GameRulesParam* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Game_getGameRulesParamByName(this->GetSkirmishAIId(), rulesParamName);
		internal_ret_int_out = springai::WrappGameRulesParam::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	springai::GameRulesParam* springai::WrappGame::GetGameRulesParamById(int rulesParamId) {

		GameRulesParam* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Game_getGameRulesParamById(this->GetSkirmishAIId(), rulesParamId);
		internal_ret_int_out = springai::WrappGameRulesParam::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	void springai::WrappGame::SendTextMessage(const char* text, int zone) {

		int internal_ret_int;

		internal_ret_int = bridged_Game_sendTextMessage(this->GetSkirmishAIId(), text, zone);
	if (internal_ret_int != 0) {
		throw CallbackAIException("sendTextMessage", internal_ret_int);
	}

	}

	void springai::WrappGame::SetLastMessagePosition(const springai::AIFloat3& pos) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Game_setLastMessagePosition(this->GetSkirmishAIId(), pos_posF3);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setLastMessagePosition", internal_ret_int);
	}

	}

	void springai::WrappGame::SendStartPosition(bool ready, const springai::AIFloat3& pos) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Game_sendStartPosition(this->GetSkirmishAIId(), ready, pos_posF3);
	if (internal_ret_int != 0) {
		throw CallbackAIException("sendStartPosition", internal_ret_int);
	}

	}

	void springai::WrappGame::SetPause(bool enable, const char* reason) {

		int internal_ret_int;

		internal_ret_int = bridged_Game_setPause(this->GetSkirmishAIId(), enable, reason);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setPause", internal_ret_int);
	}

	}
