/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappEconomy.h"

#include "IncludesSources.h"

	springai::WrappEconomy::WrappEconomy(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappEconomy::~WrappEconomy() {

	}

	int springai::WrappEconomy::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappEconomy::Economy* springai::WrappEconomy::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Economy* internal_ret = NULL;
		internal_ret = new springai::WrappEconomy(skirmishAIId);
		return internal_ret;
	}


	float springai::WrappEconomy::GetCurrent(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getCurrent(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetIncome(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getIncome(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetUsage(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getUsage(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetStorage(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getStorage(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetPull(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getPull(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetShare(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getShare(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetSent(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getSent(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetReceived(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getReceived(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappEconomy::GetExcess(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_getExcess(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	bool springai::WrappEconomy::SendResource(Resource* resource, float amount, int receivingTeamId) {

		bool internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Economy_sendResource(this->GetSkirmishAIId(), resourceId, amount, receivingTeamId);
		return internal_ret_int;
	}

	int springai::WrappEconomy::SendUnits(std::vector<springai::Unit*> unitIds_list, int receivingTeamId) {

		int unitIds_raw_size;
		int* unitIds;
		int unitIds_size;
		int internal_ret_int;

		unitIds_size = unitIds_list.size();
		int _size = unitIds_size;
		unitIds_raw_size = unitIds_size;
		unitIds = new int[unitIds_raw_size];
		for (int i=0; i < _size; ++i) {
			unitIds[i] = unitIds_list[i]->GetUnitId();
		}

		internal_ret_int = bridged_Economy_sendUnits(this->GetSkirmishAIId(), unitIds, unitIds_size, receivingTeamId);
		delete[] unitIds;

		return internal_ret_int;
	}
