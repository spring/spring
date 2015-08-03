/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubPathing.h"

#include "IncludesSources.h"

	springai::StubPathing::~StubPathing() {}
	void springai::StubPathing::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubPathing::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	int springai::StubPathing::InitPath(const springai::AIFloat3& start, const springai::AIFloat3& end, int pathType, float goalRadius) {
		return 0;
	}

	float springai::StubPathing::GetApproximateLength(const springai::AIFloat3& start, const springai::AIFloat3& end, int pathType, float goalRadius) {
		return 0;
	}

	springai::AIFloat3 springai::StubPathing::GetNextWaypoint(int pathId) {
		return springai::AIFloat3::NULL_VALUE;
	}

	void springai::StubPathing::FreePath(int pathId) {
	}

