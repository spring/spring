/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappSkirmishAIs.h"

#include "IncludesSources.h"

	springai::WrappSkirmishAIs::WrappSkirmishAIs(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappSkirmishAIs::~WrappSkirmishAIs() {

	}

	int springai::WrappSkirmishAIs::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappSkirmishAIs::SkirmishAIs* springai::WrappSkirmishAIs::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::SkirmishAIs* internal_ret = NULL;
		internal_ret = new springai::WrappSkirmishAIs(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappSkirmishAIs::GetSize() {

		int internal_ret_int;

		internal_ret_int = bridged_SkirmishAIs_getSize(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappSkirmishAIs::GetMax() {

		int internal_ret_int;

		internal_ret_int = bridged_SkirmishAIs_getMax(this->GetSkirmishAIId());
		return internal_ret_int;
	}
