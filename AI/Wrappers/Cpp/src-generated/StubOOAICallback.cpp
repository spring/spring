/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubOOAICallback.h"

#include "IncludesSources.h"

	springai::StubOOAICallback::~StubOOAICallback() {}
	void springai::StubOOAICallback::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubOOAICallback::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubOOAICallback::SetResources(std::vector<springai::Resource*> resources) {
		this->resources = resources;
	}
	std::vector<springai::Resource*> springai::StubOOAICallback::GetResources() {
		return resources;
	}

	springai::Resource* springai::StubOOAICallback::GetResourceByName(const char* resourceName) {
		return 0;
	}

	void springai::StubOOAICallback::SetUnitDefs(std::vector<springai::UnitDef*> unitDefs) {
		this->unitDefs = unitDefs;
	}
	std::vector<springai::UnitDef*> springai::StubOOAICallback::GetUnitDefs() {
		return unitDefs;
	}

	springai::UnitDef* springai::StubOOAICallback::GetUnitDefByName(const char* unitName) {
		return 0;
	}

	void springai::StubOOAICallback::SetEnemyUnits(std::vector<springai::Unit*> enemyUnits) {
		this->enemyUnits = enemyUnits;
	}
	std::vector<springai::Unit*> springai::StubOOAICallback::GetEnemyUnits() {
		return enemyUnits;
	}

	std::vector<springai::Unit*> springai::StubOOAICallback::GetEnemyUnitsIn(const springai::AIFloat3& pos, float radius) {
		return std::vector<springai::Unit*>();
	}

	void springai::StubOOAICallback::SetEnemyUnitsInRadarAndLos(std::vector<springai::Unit*> enemyUnitsInRadarAndLos) {
		this->enemyUnitsInRadarAndLos = enemyUnitsInRadarAndLos;
	}
	std::vector<springai::Unit*> springai::StubOOAICallback::GetEnemyUnitsInRadarAndLos() {
		return enemyUnitsInRadarAndLos;
	}

	void springai::StubOOAICallback::SetFriendlyUnits(std::vector<springai::Unit*> friendlyUnits) {
		this->friendlyUnits = friendlyUnits;
	}
	std::vector<springai::Unit*> springai::StubOOAICallback::GetFriendlyUnits() {
		return friendlyUnits;
	}

	std::vector<springai::Unit*> springai::StubOOAICallback::GetFriendlyUnitsIn(const springai::AIFloat3& pos, float radius) {
		return std::vector<springai::Unit*>();
	}

	void springai::StubOOAICallback::SetNeutralUnits(std::vector<springai::Unit*> neutralUnits) {
		this->neutralUnits = neutralUnits;
	}
	std::vector<springai::Unit*> springai::StubOOAICallback::GetNeutralUnits() {
		return neutralUnits;
	}

	std::vector<springai::Unit*> springai::StubOOAICallback::GetNeutralUnitsIn(const springai::AIFloat3& pos, float radius) {
		return std::vector<springai::Unit*>();
	}

	void springai::StubOOAICallback::SetTeamUnits(std::vector<springai::Unit*> teamUnits) {
		this->teamUnits = teamUnits;
	}
	std::vector<springai::Unit*> springai::StubOOAICallback::GetTeamUnits() {
		return teamUnits;
	}

	void springai::StubOOAICallback::SetSelectedUnits(std::vector<springai::Unit*> selectedUnits) {
		this->selectedUnits = selectedUnits;
	}
	std::vector<springai::Unit*> springai::StubOOAICallback::GetSelectedUnits() {
		return selectedUnits;
	}

	void springai::StubOOAICallback::SetEnemyTeams(std::vector<springai::Team*> enemyTeams) {
		this->enemyTeams = enemyTeams;
	}
	std::vector<springai::Team*> springai::StubOOAICallback::GetEnemyTeams() {
		return enemyTeams;
	}

	void springai::StubOOAICallback::SetAllyTeams(std::vector<springai::Team*> allyTeams) {
		this->allyTeams = allyTeams;
	}
	std::vector<springai::Team*> springai::StubOOAICallback::GetAllyTeams() {
		return allyTeams;
	}

	void springai::StubOOAICallback::SetGroups(std::vector<springai::Group*> groups) {
		this->groups = groups;
	}
	std::vector<springai::Group*> springai::StubOOAICallback::GetGroups() {
		return groups;
	}

	void springai::StubOOAICallback::SetFeatureDefs(std::vector<springai::FeatureDef*> featureDefs) {
		this->featureDefs = featureDefs;
	}
	std::vector<springai::FeatureDef*> springai::StubOOAICallback::GetFeatureDefs() {
		return featureDefs;
	}

	void springai::StubOOAICallback::SetFeatures(std::vector<springai::Feature*> features) {
		this->features = features;
	}
	std::vector<springai::Feature*> springai::StubOOAICallback::GetFeatures() {
		return features;
	}

	std::vector<springai::Feature*> springai::StubOOAICallback::GetFeaturesIn(const springai::AIFloat3& pos, float radius) {
		return std::vector<springai::Feature*>();
	}

	void springai::StubOOAICallback::SetWeaponDefs(std::vector<springai::WeaponDef*> weaponDefs) {
		this->weaponDefs = weaponDefs;
	}
	std::vector<springai::WeaponDef*> springai::StubOOAICallback::GetWeaponDefs() {
		return weaponDefs;
	}

	springai::WeaponDef* springai::StubOOAICallback::GetWeaponDefByName(const char* weaponDefName) {
		return 0;
	}

	void springai::StubOOAICallback::SetSkirmishAI(springai::SkirmishAI* skirmishAI) {
		this->skirmishAI = skirmishAI;
	}
	springai::SkirmishAI* springai::StubOOAICallback::GetSkirmishAI() {
		return skirmishAI;
	}

	void springai::StubOOAICallback::SetPathing(springai::Pathing* pathing) {
		this->pathing = pathing;
	}
	springai::Pathing* springai::StubOOAICallback::GetPathing() {
		return pathing;
	}

	void springai::StubOOAICallback::SetEconomy(springai::Economy* economy) {
		this->economy = economy;
	}
	springai::Economy* springai::StubOOAICallback::GetEconomy() {
		return economy;
	}

	void springai::StubOOAICallback::SetTeams(springai::Teams* teams) {
		this->teams = teams;
	}
	springai::Teams* springai::StubOOAICallback::GetTeams() {
		return teams;
	}

	void springai::StubOOAICallback::SetDataDirs(springai::DataDirs* dataDirs) {
		this->dataDirs = dataDirs;
	}
	springai::DataDirs* springai::StubOOAICallback::GetDataDirs() {
		return dataDirs;
	}

	void springai::StubOOAICallback::SetGame(springai::Game* game) {
		this->game = game;
	}
	springai::Game* springai::StubOOAICallback::GetGame() {
		return game;
	}

	void springai::StubOOAICallback::SetCheats(springai::Cheats* cheats) {
		this->cheats = cheats;
	}
	springai::Cheats* springai::StubOOAICallback::GetCheats() {
		return cheats;
	}

	void springai::StubOOAICallback::SetMap(springai::Map* map) {
		this->map = map;
	}
	springai::Map* springai::StubOOAICallback::GetMap() {
		return map;
	}

	void springai::StubOOAICallback::SetLog(springai::Log* log) {
		this->log = log;
	}
	springai::Log* springai::StubOOAICallback::GetLog() {
		return log;
	}

	void springai::StubOOAICallback::SetDebug(springai::Debug* debug) {
		this->debug = debug;
	}
	springai::Debug* springai::StubOOAICallback::GetDebug() {
		return debug;
	}

	void springai::StubOOAICallback::SetFile(springai::File* file) {
		this->file = file;
	}
	springai::File* springai::StubOOAICallback::GetFile() {
		return file;
	}

	void springai::StubOOAICallback::SetGui(springai::Gui* gui) {
		this->gui = gui;
	}
	springai::Gui* springai::StubOOAICallback::GetGui() {
		return gui;
	}

	void springai::StubOOAICallback::SetLua(springai::Lua* lua) {
		this->lua = lua;
	}
	springai::Lua* springai::StubOOAICallback::GetLua() {
		return lua;
	}

	void springai::StubOOAICallback::SetMod(springai::Mod* mod) {
		this->mod = mod;
	}
	springai::Mod* springai::StubOOAICallback::GetMod() {
		return mod;
	}

	void springai::StubOOAICallback::SetEngine(springai::Engine* engine) {
		this->engine = engine;
	}
	springai::Engine* springai::StubOOAICallback::GetEngine() {
		return engine;
	}

	void springai::StubOOAICallback::SetSkirmishAIs(springai::SkirmishAIs* skirmishAIs) {
		this->skirmishAIs = skirmishAIs;
	}
	springai::SkirmishAIs* springai::StubOOAICallback::GetSkirmishAIs() {
		return skirmishAIs;
	}

