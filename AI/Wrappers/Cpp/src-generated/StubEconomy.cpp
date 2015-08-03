/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubEconomy.h"

#include "IncludesSources.h"

	springai::StubEconomy::~StubEconomy() {}
	void springai::StubEconomy::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubEconomy::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	float springai::StubEconomy::GetCurrent(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetIncome(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetUsage(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetStorage(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetPull(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetShare(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetSent(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetReceived(Resource* resource) {
		return 0;
	}

	float springai::StubEconomy::GetExcess(Resource* resource) {
		return 0;
	}

	bool springai::StubEconomy::SendResource(Resource* resource, float amount, int receivingTeamId) {
		return false;
	}

	int springai::StubEconomy::SendUnits(std::vector<springai::Unit*> unitIds_list, int receivingTeamId) {
		return 0;
	}

