/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappOOAICallback.h"

#include "IncludesSources.h"

struct SSkirmishAICallback;

	springai::WrappOOAICallback::WrappOOAICallback(const struct SSkirmishAICallback* innerCallback, int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
		funcPntBrdg_addCallback(skirmishAIId, innerCallback);
	}

	springai::WrappOOAICallback::~WrappOOAICallback() {

		funcPntBrdg_removeCallback(skirmishAIId);
	}

	int springai::WrappOOAICallback::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappOOAICallback::OOAICallback* springai::WrappOOAICallback::GetInstance(const struct SSkirmishAICallback* innerCallback, int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::OOAICallback* internal_ret = NULL;
		internal_ret = new springai::WrappOOAICallback(innerCallback, skirmishAIId);
		return internal_ret;
	}


	std::vector<springai::Resource*> springai::WrappOOAICallback::GetResources() {

		std::vector<springai::Resource*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_getResources(this->GetSkirmishAIId());
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappResource::GetInstance(skirmishAIId, i));
		}

		return RETURN_SIZE_list;
	}

	springai::Resource* springai::WrappOOAICallback::GetResourceByName(const char* resourceName) {

		Resource* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_getResourceByName(this->GetSkirmishAIId(), resourceName);
		internal_ret_int_out = springai::WrappResource::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	std::vector<springai::UnitDef*> springai::WrappOOAICallback::GetUnitDefs() {

		int unitDefIds_sizeMax;
		int unitDefIds_raw_size;
		int* unitDefIds;
		std::vector<springai::UnitDef*> unitDefIds_list;
		int unitDefIds_size;
		int internal_ret_int;

		unitDefIds_sizeMax = INT_MAX;
		unitDefIds = NULL;
		unitDefIds_size = bridged_getUnitDefs(this->GetSkirmishAIId(), unitDefIds, unitDefIds_sizeMax);
		unitDefIds_sizeMax = unitDefIds_size;
		unitDefIds_raw_size = unitDefIds_size;
		unitDefIds = new int[unitDefIds_raw_size];

		internal_ret_int = bridged_getUnitDefs(this->GetSkirmishAIId(), unitDefIds, unitDefIds_sizeMax);
		unitDefIds_list.reserve(unitDefIds_size);
		for (int i=0; i < unitDefIds_sizeMax; ++i) {
			unitDefIds_list.push_back(springai::WrappUnitDef::GetInstance(skirmishAIId, unitDefIds[i]));
		}
		delete[] unitDefIds;

		return unitDefIds_list;
	}

	springai::UnitDef* springai::WrappOOAICallback::GetUnitDefByName(const char* unitName) {

		UnitDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_getUnitDefByName(this->GetSkirmishAIId(), unitName);
		internal_ret_int_out = springai::WrappUnitDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetEnemyUnits() {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getEnemyUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getEnemyUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetEnemyUnitsIn(const springai::AIFloat3& pos, float radius) {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getEnemyUnitsIn(this->GetSkirmishAIId(), pos_posF3, radius, unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getEnemyUnitsIn(this->GetSkirmishAIId(), pos_posF3, radius, unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetEnemyUnitsInRadarAndLos() {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getEnemyUnitsInRadarAndLos(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getEnemyUnitsInRadarAndLos(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetFriendlyUnits() {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getFriendlyUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getFriendlyUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetFriendlyUnitsIn(const springai::AIFloat3& pos, float radius) {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getFriendlyUnitsIn(this->GetSkirmishAIId(), pos_posF3, radius, unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getFriendlyUnitsIn(this->GetSkirmishAIId(), pos_posF3, radius, unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetNeutralUnits() {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getNeutralUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getNeutralUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetNeutralUnitsIn(const springai::AIFloat3& pos, float radius) {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getNeutralUnitsIn(this->GetSkirmishAIId(), pos_posF3, radius, unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getNeutralUnitsIn(this->GetSkirmishAIId(), pos_posF3, radius, unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetTeamUnits() {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getTeamUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getTeamUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Unit*> springai::WrappOOAICallback::GetSelectedUnits() {

		int unitIds_sizeMax;
		int unitIds_raw_size;
		int* unitIds;
		std::vector<springai::Unit*> unitIds_list;
		int unitIds_size;
		int internal_ret_int;

		unitIds_sizeMax = INT_MAX;
		unitIds = NULL;
		unitIds_size = bridged_getSelectedUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_sizeMax = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];

		internal_ret_int = bridged_getSelectedUnits(this->GetSkirmishAIId(), unitIds, unitIds_sizeMax);
		unitIds_list.reserve(unitIds_size);
		for (int i=0; i < unitIds_sizeMax; ++i) {
			unitIds_list.push_back(springai::WrappUnit::GetInstance(skirmishAIId, unitIds[i]));
		}
		delete[] unitIds;

		return unitIds_list;
	}

	std::vector<springai::Team*> springai::WrappOOAICallback::GetEnemyTeams() {

		int teamIds_sizeMax;
		int teamIds_raw_size;
		int* teamIds;
		std::vector<springai::Team*> teamIds_list;
		int teamIds_size;
		int internal_ret_int;

		teamIds_sizeMax = INT_MAX;
		teamIds = NULL;
		teamIds_size = bridged_getEnemyTeams(this->GetSkirmishAIId(), teamIds, teamIds_sizeMax);
		teamIds_sizeMax = teamIds_size;
		teamIds_raw_size = teamIds_size;
		teamIds = new int[teamIds_raw_size];

		internal_ret_int = bridged_getEnemyTeams(this->GetSkirmishAIId(), teamIds, teamIds_sizeMax);
		teamIds_list.reserve(teamIds_size);
		for (int i=0; i < teamIds_sizeMax; ++i) {
			teamIds_list.push_back(springai::WrappTeam::GetInstance(skirmishAIId, teamIds[i]));
		}
		delete[] teamIds;

		return teamIds_list;
	}

	std::vector<springai::Team*> springai::WrappOOAICallback::GetAllyTeams() {

		int teamIds_sizeMax;
		int teamIds_raw_size;
		int* teamIds;
		std::vector<springai::Team*> teamIds_list;
		int teamIds_size;
		int internal_ret_int;

		teamIds_sizeMax = INT_MAX;
		teamIds = NULL;
		teamIds_size = bridged_getAllyTeams(this->GetSkirmishAIId(), teamIds, teamIds_sizeMax);
		teamIds_sizeMax = teamIds_size;
		teamIds_raw_size = teamIds_size;
		teamIds = new int[teamIds_raw_size];

		internal_ret_int = bridged_getAllyTeams(this->GetSkirmishAIId(), teamIds, teamIds_sizeMax);
		teamIds_list.reserve(teamIds_size);
		for (int i=0; i < teamIds_sizeMax; ++i) {
			teamIds_list.push_back(springai::WrappTeam::GetInstance(skirmishAIId, teamIds[i]));
		}
		delete[] teamIds;

		return teamIds_list;
	}

	std::vector<springai::Group*> springai::WrappOOAICallback::GetGroups() {

		int groupIds_sizeMax;
		int groupIds_raw_size;
		int* groupIds;
		std::vector<springai::Group*> groupIds_list;
		int groupIds_size;
		int internal_ret_int;

		groupIds_sizeMax = INT_MAX;
		groupIds = NULL;
		groupIds_size = bridged_getGroups(this->GetSkirmishAIId(), groupIds, groupIds_sizeMax);
		groupIds_sizeMax = groupIds_size;
		groupIds_raw_size = groupIds_size;
		groupIds = new int[groupIds_raw_size];

		internal_ret_int = bridged_getGroups(this->GetSkirmishAIId(), groupIds, groupIds_sizeMax);
		groupIds_list.reserve(groupIds_size);
		for (int i=0; i < groupIds_sizeMax; ++i) {
			groupIds_list.push_back(springai::WrappGroup::GetInstance(skirmishAIId, groupIds[i]));
		}
		delete[] groupIds;

		return groupIds_list;
	}

	std::vector<springai::FeatureDef*> springai::WrappOOAICallback::GetFeatureDefs() {

		int featureDefIds_sizeMax;
		int featureDefIds_raw_size;
		int* featureDefIds;
		std::vector<springai::FeatureDef*> featureDefIds_list;
		int featureDefIds_size;
		int internal_ret_int;

		featureDefIds_sizeMax = INT_MAX;
		featureDefIds = NULL;
		featureDefIds_size = bridged_getFeatureDefs(this->GetSkirmishAIId(), featureDefIds, featureDefIds_sizeMax);
		featureDefIds_sizeMax = featureDefIds_size;
		featureDefIds_raw_size = featureDefIds_size;
		featureDefIds = new int[featureDefIds_raw_size];

		internal_ret_int = bridged_getFeatureDefs(this->GetSkirmishAIId(), featureDefIds, featureDefIds_sizeMax);
		featureDefIds_list.reserve(featureDefIds_size);
		for (int i=0; i < featureDefIds_sizeMax; ++i) {
			featureDefIds_list.push_back(springai::WrappFeatureDef::GetInstance(skirmishAIId, featureDefIds[i]));
		}
		delete[] featureDefIds;

		return featureDefIds_list;
	}

	std::vector<springai::Feature*> springai::WrappOOAICallback::GetFeatures() {

		int featureIds_sizeMax;
		int featureIds_raw_size;
		int* featureIds;
		std::vector<springai::Feature*> featureIds_list;
		int featureIds_size;
		int internal_ret_int;

		featureIds_sizeMax = INT_MAX;
		featureIds = NULL;
		featureIds_size = bridged_getFeatures(this->GetSkirmishAIId(), featureIds, featureIds_sizeMax);
		featureIds_sizeMax = featureIds_size;
		featureIds_raw_size = featureIds_size;
		featureIds = new int[featureIds_raw_size];

		internal_ret_int = bridged_getFeatures(this->GetSkirmishAIId(), featureIds, featureIds_sizeMax);
		featureIds_list.reserve(featureIds_size);
		for (int i=0; i < featureIds_sizeMax; ++i) {
			featureIds_list.push_back(springai::WrappFeature::GetInstance(skirmishAIId, featureIds[i]));
		}
		delete[] featureIds;

		return featureIds_list;
	}

	std::vector<springai::Feature*> springai::WrappOOAICallback::GetFeaturesIn(const springai::AIFloat3& pos, float radius) {

		int featureIds_sizeMax;
		int featureIds_raw_size;
		int* featureIds;
		std::vector<springai::Feature*> featureIds_list;
		int featureIds_size;
		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		featureIds_sizeMax = INT_MAX;
		featureIds = NULL;
		featureIds_size = bridged_getFeaturesIn(this->GetSkirmishAIId(), pos_posF3, radius, featureIds, featureIds_sizeMax);
		featureIds_sizeMax = featureIds_size;
		featureIds_raw_size = featureIds_size;
		featureIds = new int[featureIds_raw_size];

		internal_ret_int = bridged_getFeaturesIn(this->GetSkirmishAIId(), pos_posF3, radius, featureIds, featureIds_sizeMax);
		featureIds_list.reserve(featureIds_size);
		for (int i=0; i < featureIds_sizeMax; ++i) {
			featureIds_list.push_back(springai::WrappFeature::GetInstance(skirmishAIId, featureIds[i]));
		}
		delete[] featureIds;

		return featureIds_list;
	}

	std::vector<springai::WeaponDef*> springai::WrappOOAICallback::GetWeaponDefs() {

		std::vector<springai::WeaponDef*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_getWeaponDefs(this->GetSkirmishAIId());
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappWeaponDef::GetInstance(skirmishAIId, i));
		}

		return RETURN_SIZE_list;
	}

	springai::WeaponDef* springai::WrappOOAICallback::GetWeaponDefByName(const char* weaponDefName) {

		WeaponDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_getWeaponDefByName(this->GetSkirmishAIId(), weaponDefName);
		internal_ret_int_out = springai::WrappWeaponDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	springai::SkirmishAI* springai::WrappOOAICallback::GetSkirmishAI() {

		SkirmishAI* internal_ret_int_out;

		internal_ret_int_out = springai::WrappSkirmishAI::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Pathing* springai::WrappOOAICallback::GetPathing() {

		Pathing* internal_ret_int_out;

		internal_ret_int_out = springai::WrappPathing::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Economy* springai::WrappOOAICallback::GetEconomy() {

		Economy* internal_ret_int_out;

		internal_ret_int_out = springai::WrappEconomy::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Teams* springai::WrappOOAICallback::GetTeams() {

		Teams* internal_ret_int_out;

		internal_ret_int_out = springai::WrappTeams::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::DataDirs* springai::WrappOOAICallback::GetDataDirs() {

		DataDirs* internal_ret_int_out;

		internal_ret_int_out = springai::WrappDataDirs::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Game* springai::WrappOOAICallback::GetGame() {

		Game* internal_ret_int_out;

		internal_ret_int_out = springai::WrappGame::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Cheats* springai::WrappOOAICallback::GetCheats() {

		Cheats* internal_ret_int_out;

		internal_ret_int_out = springai::WrappCheats::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Map* springai::WrappOOAICallback::GetMap() {

		Map* internal_ret_int_out;

		internal_ret_int_out = springai::WrappMap::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Log* springai::WrappOOAICallback::GetLog() {

		Log* internal_ret_int_out;

		internal_ret_int_out = springai::WrappLog::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Debug* springai::WrappOOAICallback::GetDebug() {

		Debug* internal_ret_int_out;

		internal_ret_int_out = springai::WrappDebug::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::File* springai::WrappOOAICallback::GetFile() {

		File* internal_ret_int_out;

		internal_ret_int_out = springai::WrappFile::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Gui* springai::WrappOOAICallback::GetGui() {

		Gui* internal_ret_int_out;

		internal_ret_int_out = springai::WrappGui::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Lua* springai::WrappOOAICallback::GetLua() {

		Lua* internal_ret_int_out;

		internal_ret_int_out = springai::WrappLua::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Mod* springai::WrappOOAICallback::GetMod() {

		Mod* internal_ret_int_out;

		internal_ret_int_out = springai::WrappMod::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Engine* springai::WrappOOAICallback::GetEngine() {

		Engine* internal_ret_int_out;

		internal_ret_int_out = springai::WrappEngine::GetInstance();

		return internal_ret_int_out;
	}

	springai::SkirmishAIs* springai::WrappOOAICallback::GetSkirmishAIs() {

		SkirmishAIs* internal_ret_int_out;

		internal_ret_int_out = springai::WrappSkirmishAIs::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}
