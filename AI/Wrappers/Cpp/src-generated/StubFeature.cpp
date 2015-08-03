/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubFeature.h"

#include "IncludesSources.h"

	springai::StubFeature::~StubFeature() {}
	void springai::StubFeature::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubFeature::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubFeature::SetFeatureId(int featureId) {
		this->featureId = featureId;
	}
	int springai::StubFeature::GetFeatureId() const {
		return featureId;
	}

	void springai::StubFeature::SetDef(springai::FeatureDef* def) {
		this->def = def;
	}
	springai::FeatureDef* springai::StubFeature::GetDef() {
		return def;
	}

	void springai::StubFeature::SetHealth(float health) {
		this->health = health;
	}
	float springai::StubFeature::GetHealth() {
		return health;
	}

	void springai::StubFeature::SetReclaimLeft(float reclaimLeft) {
		this->reclaimLeft = reclaimLeft;
	}
	float springai::StubFeature::GetReclaimLeft() {
		return reclaimLeft;
	}

	void springai::StubFeature::SetPosition(springai::AIFloat3 position) {
		this->position = position;
	}
	springai::AIFloat3 springai::StubFeature::GetPosition() {
		return position;
	}

