/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappTeams.h"

#include "IncludesSources.h"

	springai::WrappTeams::WrappTeams(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappTeams::~WrappTeams() {

	}

	int springai::WrappTeams::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappTeams::Teams* springai::WrappTeams::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Teams* internal_ret = NULL;
		internal_ret = new springai::WrappTeams(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappTeams::GetSize() {

		int internal_ret_int;

		internal_ret_int = bridged_Teams_getSize(this->GetSkirmishAIId());
		return internal_ret_int;
	}
