/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappSkirmishAI.h"

#include "IncludesSources.h"

	springai::WrappSkirmishAI::WrappSkirmishAI(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappSkirmishAI::~WrappSkirmishAI() {

	}

	int springai::WrappSkirmishAI::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappSkirmishAI::SkirmishAI* springai::WrappSkirmishAI::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::SkirmishAI* internal_ret = NULL;
		internal_ret = new springai::WrappSkirmishAI(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappSkirmishAI::GetTeamId() {

		int internal_ret_int;

		internal_ret_int = bridged_SkirmishAI_getTeamId(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	springai::OptionValues* springai::WrappSkirmishAI::GetOptionValues() {

		OptionValues* internal_ret_int_out;

		internal_ret_int_out = springai::WrappOptionValues::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::Info* springai::WrappSkirmishAI::GetInfo() {

		Info* internal_ret_int_out;

		internal_ret_int_out = springai::WrappInfo::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}
