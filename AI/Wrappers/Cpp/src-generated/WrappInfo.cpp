/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappInfo.h"

#include "IncludesSources.h"

	springai::WrappInfo::WrappInfo(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappInfo::~WrappInfo() {

	}

	int springai::WrappInfo::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappInfo::Info* springai::WrappInfo::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Info* internal_ret = NULL;
		internal_ret = new springai::WrappInfo(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappInfo::GetSize() {

		int internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_Info_getSize(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappInfo::GetKey(int infoIndex) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_Info_getKey(this->GetSkirmishAIId(), infoIndex);
		return internal_ret_int;
	}

	const char* springai::WrappInfo::GetValue(int infoIndex) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_Info_getValue(this->GetSkirmishAIId(), infoIndex);
		return internal_ret_int;
	}

	const char* springai::WrappInfo::GetDescription(int infoIndex) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_Info_getDescription(this->GetSkirmishAIId(), infoIndex);
		return internal_ret_int;
	}

	const char* springai::WrappInfo::GetValueByKey(const char* const key) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_Info_getValueByKey(this->GetSkirmishAIId(), key);
		return internal_ret_int;
	}
