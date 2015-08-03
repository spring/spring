/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubGame.h"

#include "IncludesSources.h"

	springai::StubGame::~StubGame() {}
	void springai::StubGame::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubGame::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubGame::SetCurrentFrame(int currentFrame) {
		this->currentFrame = currentFrame;
	}
	int springai::StubGame::GetCurrentFrame() {
		return currentFrame;
	}

	void springai::StubGame::SetAiInterfaceVersion(int aiInterfaceVersion) {
		this->aiInterfaceVersion = aiInterfaceVersion;
	}
	int springai::StubGame::GetAiInterfaceVersion() {
		return aiInterfaceVersion;
	}

	void springai::StubGame::SetMyTeam(int myTeam) {
		this->myTeam = myTeam;
	}
	int springai::StubGame::GetMyTeam() {
		return myTeam;
	}

	void springai::StubGame::SetMyAllyTeam(int myAllyTeam) {
		this->myAllyTeam = myAllyTeam;
	}
	int springai::StubGame::GetMyAllyTeam() {
		return myAllyTeam;
	}

	int springai::StubGame::GetPlayerTeam(int playerId) {
		return 0;
	}

	void springai::StubGame::SetTeams(int teams) {
		this->teams = teams;
	}
	int springai::StubGame::GetTeams() {
		return teams;
	}

	const char* springai::StubGame::GetTeamSide(int otherTeamId) {
		return 0;
	}

	springai::AIColor springai::StubGame::GetTeamColor(int otherTeamId) {
		return springai::AIColor::NULL_VALUE;
	}

	float springai::StubGame::GetTeamIncomeMultiplier(int otherTeamId) {
		return 0;
	}

	int springai::StubGame::GetTeamAllyTeam(int otherTeamId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceCurrent(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceIncome(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceUsage(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceStorage(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourcePull(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceShare(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceSent(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceReceived(int otherTeamId, int resourceId) {
		return 0;
	}

	float springai::StubGame::GetTeamResourceExcess(int otherTeamId, int resourceId) {
		return 0;
	}

	bool springai::StubGame::IsAllied(int firstAllyTeamId, int secondAllyTeamId) {
		return false;
	}

	void springai::StubGame::SetExceptionHandlingEnabled(bool isExceptionHandlingEnabled) {
		this->isExceptionHandlingEnabled = isExceptionHandlingEnabled;
	}
	bool springai::StubGame::IsExceptionHandlingEnabled() {
		return isExceptionHandlingEnabled;
	}

	void springai::StubGame::SetDebugModeEnabled(bool isDebugModeEnabled) {
		this->isDebugModeEnabled = isDebugModeEnabled;
	}
	bool springai::StubGame::IsDebugModeEnabled() {
		return isDebugModeEnabled;
	}

	void springai::StubGame::SetMode(int mode) {
		this->mode = mode;
	}
	int springai::StubGame::GetMode() {
		return mode;
	}

	void springai::StubGame::SetPaused(bool isPaused) {
		this->isPaused = isPaused;
	}
	bool springai::StubGame::IsPaused() {
		return isPaused;
	}

	void springai::StubGame::SetSpeedFactor(float speedFactor) {
		this->speedFactor = speedFactor;
	}
	float springai::StubGame::GetSpeedFactor() {
		return speedFactor;
	}

	void springai::StubGame::SetSetupScript(const char* setupScript) {
		this->setupScript = setupScript;
	}
	const char* springai::StubGame::GetSetupScript() {
		return setupScript;
	}

	int springai::StubGame::GetCategoryFlag(const char* categoryName) {
		return 0;
	}

	int springai::StubGame::GetCategoriesFlag(const char* categoryNames) {
		return 0;
	}

	void springai::StubGame::GetCategoryName(int categoryFlag, char* name, int name_sizeMax) {
	}

	void springai::StubGame::SetGameRulesParams(std::vector<springai::GameRulesParam*> gameRulesParams) {
		this->gameRulesParams = gameRulesParams;
	}
	std::vector<springai::GameRulesParam*> springai::StubGame::GetGameRulesParams() {
		return gameRulesParams;
	}

	springai::GameRulesParam* springai::StubGame::GetGameRulesParamByName(const char* rulesParamName) {
		return 0;
	}

	springai::GameRulesParam* springai::StubGame::GetGameRulesParamById(int rulesParamId) {
		return 0;
	}

	void springai::StubGame::SendTextMessage(const char* text, int zone) {
	}

	void springai::StubGame::SetLastMessagePosition(const springai::AIFloat3& pos) {
	}

	void springai::StubGame::SendStartPosition(bool ready, const springai::AIFloat3& pos) {
	}

	void springai::StubGame::SetPause(bool enable, const char* reason) {
	}

