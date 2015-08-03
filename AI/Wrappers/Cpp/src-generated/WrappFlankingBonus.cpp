/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappFlankingBonus.h"

#include "IncludesSources.h"

	springai::WrappFlankingBonus::WrappFlankingBonus(int skirmishAIId, int unitDefId) {

		this->skirmishAIId = skirmishAIId;
		this->unitDefId = unitDefId;
	}

	springai::WrappFlankingBonus::~WrappFlankingBonus() {

	}

	int springai::WrappFlankingBonus::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappFlankingBonus::GetUnitDefId() const {

		return unitDefId;
	}

	springai::WrappFlankingBonus::FlankingBonus* springai::WrappFlankingBonus::GetInstance(int skirmishAIId, int unitDefId) {

		if (unitDefId < 0) {
			return NULL;
		}

		springai::FlankingBonus* internal_ret = NULL;
		internal_ret = new springai::WrappFlankingBonus(skirmishAIId, unitDefId);
		return internal_ret;
	}


	int springai::WrappFlankingBonus::GetMode() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_FlankingBonus_getMode(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappFlankingBonus::GetDir() {

		float return_posF3_out[3];

		bridged_UnitDef_FlankingBonus_getDir(this->GetSkirmishAIId(), this->GetUnitDefId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	float springai::WrappFlankingBonus::GetMax() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_FlankingBonus_getMax(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappFlankingBonus::GetMin() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_FlankingBonus_getMin(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappFlankingBonus::GetMobilityAdd() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_FlankingBonus_getMobilityAdd(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}
