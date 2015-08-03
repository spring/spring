/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappMoveData.h"

#include "IncludesSources.h"

	springai::WrappMoveData::WrappMoveData(int skirmishAIId, int unitDefId) {

		this->skirmishAIId = skirmishAIId;
		this->unitDefId = unitDefId;
	}

	springai::WrappMoveData::~WrappMoveData() {

	}

	int springai::WrappMoveData::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappMoveData::GetUnitDefId() const {

		return unitDefId;
	}

	springai::WrappMoveData::MoveData* springai::WrappMoveData::GetInstance(int skirmishAIId, int unitDefId) {

		if (unitDefId < 0) {
			return NULL;
		}

		springai::MoveData* internal_ret = NULL;
		bool isAvailable = bridged_UnitDef_isMoveDataAvailable(skirmishAIId, unitDefId);
		if (isAvailable) {
			internal_ret = new springai::WrappMoveData(skirmishAIId, unitDefId);
		}
		return internal_ret;
	}


	float springai::WrappMoveData::GetMaxAcceleration() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getMaxAcceleration(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetMaxBreaking() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getMaxBreaking(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetMaxSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getMaxSpeed(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	short springai::WrappMoveData::GetMaxTurnRate() {

		short internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getMaxTurnRate(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappMoveData::GetXSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getXSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappMoveData::GetZSize() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getZSize(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetDepth() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getDepth(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetMaxSlope() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getMaxSlope(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetSlopeMod() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getSlopeMod(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetDepthMod(float height) {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getDepthMod(this->GetSkirmishAIId(), this->GetUnitDefId(), height);
		return internal_ret_int;
	}

	int springai::WrappMoveData::GetPathType() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getPathType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	float springai::WrappMoveData::GetCrushStrength() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getCrushStrength(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappMoveData::GetMoveType() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getMoveType(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappMoveData::GetSpeedModClass() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getSpeedModClass(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	int springai::WrappMoveData::GetTerrainClass() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getTerrainClass(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappMoveData::GetFollowGround() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getFollowGround(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	bool springai::WrappMoveData::IsSubMarine() {

		bool internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_isSubMarine(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}

	const char* springai::WrappMoveData::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_MoveData_getName(this->GetSkirmishAIId(), this->GetUnitDefId());
		return internal_ret_int;
	}
