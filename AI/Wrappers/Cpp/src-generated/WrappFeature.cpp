/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappFeature.h"

#include "IncludesSources.h"

	springai::WrappFeature::WrappFeature(int skirmishAIId, int featureId) {

		this->skirmishAIId = skirmishAIId;
		this->featureId = featureId;
	}

	springai::WrappFeature::~WrappFeature() {

	}

	int springai::WrappFeature::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappFeature::GetFeatureId() const {

		return featureId;
	}

	springai::WrappFeature::Feature* springai::WrappFeature::GetInstance(int skirmishAIId, int featureId) {

		if (featureId < 0) {
			return NULL;
		}

		springai::Feature* internal_ret = NULL;
		internal_ret = new springai::WrappFeature(skirmishAIId, featureId);
		return internal_ret;
	}


	springai::FeatureDef* springai::WrappFeature::GetDef() {

		FeatureDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Feature_getDef(this->GetSkirmishAIId(), this->GetFeatureId());
		internal_ret_int_out = springai::WrappFeatureDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	float springai::WrappFeature::GetHealth() {

		float internal_ret_int;

		internal_ret_int = bridged_Feature_getHealth(this->GetSkirmishAIId(), this->GetFeatureId());
		return internal_ret_int;
	}

	float springai::WrappFeature::GetReclaimLeft() {

		float internal_ret_int;

		internal_ret_int = bridged_Feature_getReclaimLeft(this->GetSkirmishAIId(), this->GetFeatureId());
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappFeature::GetPosition() {

		float return_posF3_out[3];

		bridged_Feature_getPosition(this->GetSkirmishAIId(), this->GetFeatureId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}
