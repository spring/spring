/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappOptionValues.h"

#include "IncludesSources.h"

	springai::WrappOptionValues::WrappOptionValues(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappOptionValues::~WrappOptionValues() {

	}

	int springai::WrappOptionValues::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappOptionValues::OptionValues* springai::WrappOptionValues::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::OptionValues* internal_ret = NULL;
		internal_ret = new springai::WrappOptionValues(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappOptionValues::GetSize() {

		int internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_OptionValues_getSize(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappOptionValues::GetKey(int optionIndex) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_OptionValues_getKey(this->GetSkirmishAIId(), optionIndex);
		return internal_ret_int;
	}

	const char* springai::WrappOptionValues::GetValue(int optionIndex) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_OptionValues_getValue(this->GetSkirmishAIId(), optionIndex);
		return internal_ret_int;
	}

	const char* springai::WrappOptionValues::GetValueByKey(const char* const key) {

		const char* internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_OptionValues_getValueByKey(this->GetSkirmishAIId(), key);
		return internal_ret_int;
	}
