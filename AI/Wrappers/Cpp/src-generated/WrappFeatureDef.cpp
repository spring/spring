/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappFeatureDef.h"

#include "IncludesSources.h"

	springai::WrappFeatureDef::WrappFeatureDef(int skirmishAIId, int featureDefId) {

		this->skirmishAIId = skirmishAIId;
		this->featureDefId = featureDefId;
	}

	springai::WrappFeatureDef::~WrappFeatureDef() {

	}

	int springai::WrappFeatureDef::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappFeatureDef::GetFeatureDefId() const {

		return featureDefId;
	}

	springai::WrappFeatureDef::FeatureDef* springai::WrappFeatureDef::GetInstance(int skirmishAIId, int featureDefId) {

		if (featureDefId < 0) {
			return NULL;
		}

		springai::FeatureDef* internal_ret = NULL;
		internal_ret = new springai::WrappFeatureDef(skirmishAIId, featureDefId);
		return internal_ret;
	}


	const char* springai::WrappFeatureDef::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getName(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	const char* springai::WrappFeatureDef::GetDescription() {

		const char* internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getDescription(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	const char* springai::WrappFeatureDef::GetFileName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getFileName(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	float springai::WrappFeatureDef::GetContainedResource(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_FeatureDef_getContainedResource(this->GetSkirmishAIId(), this->GetFeatureDefId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappFeatureDef::GetMaxHealth() {

		float internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getMaxHealth(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	float springai::WrappFeatureDef::GetReclaimTime() {

		float internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getReclaimTime(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	float springai::WrappFeatureDef::GetMass() {

		float internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getMass(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsUpright() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isUpright(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	int springai::WrappFeatureDef::GetDrawType() {

		int internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getDrawType(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	const char* springai::WrappFeatureDef::GetModelName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getModelName(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	int springai::WrappFeatureDef::GetResurrectable() {

		int internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getResurrectable(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	int springai::WrappFeatureDef::GetSmokeTime() {

		int internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getSmokeTime(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsDestructable() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isDestructable(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsReclaimable() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isReclaimable(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsBlocking() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isBlocking(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsBurnable() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isBurnable(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsFloating() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isFloating(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsNoSelect() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isNoSelect(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	bool springai::WrappFeatureDef::IsGeoThermal() {

		bool internal_ret_int;

		internal_ret_int = bridged_FeatureDef_isGeoThermal(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	const char* springai::WrappFeatureDef::GetDeathFeature() {

		const char* internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getDeathFeature(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	int springai::WrappFeatureDef::GetXSize() {

		int internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getXSize(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	int springai::WrappFeatureDef::GetZSize() {

		int internal_ret_int;

		internal_ret_int = bridged_FeatureDef_getZSize(this->GetSkirmishAIId(), this->GetFeatureDefId());
		return internal_ret_int;
	}

	std::map<std::string,std::string> springai::WrappFeatureDef::GetCustomParams() {

		std::map<std::string,std::string> internal_map;
		int internal_size;
		int internal_ret_int;

		internal_size = bridged_FeatureDef_getCustomParams(this->GetSkirmishAIId(), this->GetFeatureDefId(), NULL, NULL);
		const char** keys = new const char*[internal_size];
		const char** values = new const char*[internal_size];

		internal_ret_int = bridged_FeatureDef_getCustomParams(this->GetSkirmishAIId(), this->GetFeatureDefId(), keys, values);
		for (int i=0; i < internal_size; i++) {
			internal_map[keys[i]] = values[i];
		}

		delete[] keys;
		delete[] values;

		return internal_map;
	}
