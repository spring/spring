/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubCamera.h"

#include "IncludesSources.h"

	springai::StubCamera::~StubCamera() {}
	void springai::StubCamera::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubCamera::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubCamera::SetDirection(springai::AIFloat3 direction) {
		this->direction = direction;
	}
	springai::AIFloat3 springai::StubCamera::GetDirection() {
		return direction;
	}

	void springai::StubCamera::SetPosition(springai::AIFloat3 position) {
		this->position = position;
	}
	springai::AIFloat3 springai::StubCamera::GetPosition() {
		return position;
	}

