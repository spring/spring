/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubCheats.h"

#include "IncludesSources.h"

	springai::StubCheats::~StubCheats() {}
	void springai::StubCheats::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubCheats::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	bool springai::StubCheats::IsEnabled() {
		return false;
	}

	bool springai::StubCheats::SetEnabled(bool enable) {
		return false;
	}

	bool springai::StubCheats::SetEventsEnabled(bool enabled) {
		return false;
	}

	void springai::StubCheats::SetOnlyPassive(bool isOnlyPassive) {
		this->isOnlyPassive = isOnlyPassive;
	}
	bool springai::StubCheats::IsOnlyPassive() {
		return isOnlyPassive;
	}

	void springai::StubCheats::SetMyIncomeMultiplier(float factor) {
	}

	void springai::StubCheats::GiveMeResource(Resource* resource, float amount) {
	}

	int springai::StubCheats::GiveMeUnit(UnitDef* unitDef, const springai::AIFloat3& pos) {
		return 0;
	}

