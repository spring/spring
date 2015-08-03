/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappPathing.h"

#include "IncludesSources.h"

	springai::WrappPathing::WrappPathing(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappPathing::~WrappPathing() {

	}

	int springai::WrappPathing::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappPathing::Pathing* springai::WrappPathing::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Pathing* internal_ret = NULL;
		internal_ret = new springai::WrappPathing(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappPathing::InitPath(const springai::AIFloat3& start, const springai::AIFloat3& end, int pathType, float goalRadius) {

		int internal_ret_int;

		float start_posF3[3];
		start.LoadInto(start_posF3);
		float end_posF3[3];
		end.LoadInto(end_posF3);

		internal_ret_int = bridged_Pathing_initPath(this->GetSkirmishAIId(), start_posF3, end_posF3, pathType, goalRadius);
		return internal_ret_int;
	}

	float springai::WrappPathing::GetApproximateLength(const springai::AIFloat3& start, const springai::AIFloat3& end, int pathType, float goalRadius) {

		float internal_ret_int;

		float start_posF3[3];
		start.LoadInto(start_posF3);
		float end_posF3[3];
		end.LoadInto(end_posF3);

		internal_ret_int = bridged_Pathing_getApproximateLength(this->GetSkirmishAIId(), start_posF3, end_posF3, pathType, goalRadius);
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappPathing::GetNextWaypoint(int pathId) {

		int internal_ret_int;

		float ret_nextWaypoint_posF3_out[3];

		internal_ret_int = bridged_Pathing_getNextWaypoint(this->GetSkirmishAIId(), pathId, ret_nextWaypoint_posF3_out);
	if (internal_ret_int != 0) {
		throw CallbackAIException("getNextWaypoint", internal_ret_int);
	}
		springai::AIFloat3 internal_ret(ret_nextWaypoint_posF3_out[0], ret_nextWaypoint_posF3_out[1], ret_nextWaypoint_posF3_out[2]);

		return internal_ret;
	}

	void springai::WrappPathing::FreePath(int pathId) {

		int internal_ret_int;

		internal_ret_int = bridged_Pathing_freePath(this->GetSkirmishAIId(), pathId);
	if (internal_ret_int != 0) {
		throw CallbackAIException("freePath", internal_ret_int);
	}

	}
