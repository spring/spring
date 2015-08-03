/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubGui.h"

#include "IncludesSources.h"

	springai::StubGui::~StubGui() {}
	void springai::StubGui::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubGui::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubGui::SetViewRange(float viewRange) {
		this->viewRange = viewRange;
	}
	float springai::StubGui::GetViewRange() {
		return viewRange;
	}

	void springai::StubGui::SetScreenX(float screenX) {
		this->screenX = screenX;
	}
	float springai::StubGui::GetScreenX() {
		return screenX;
	}

	void springai::StubGui::SetScreenY(float screenY) {
		this->screenY = screenY;
	}
	float springai::StubGui::GetScreenY() {
		return screenY;
	}

	void springai::StubGui::SetCamera(springai::Camera* camera) {
		this->camera = camera;
	}
	springai::Camera* springai::StubGui::GetCamera() {
		return camera;
	}

