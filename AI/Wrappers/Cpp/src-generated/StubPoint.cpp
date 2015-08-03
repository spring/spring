/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubPoint.h"

#include "IncludesSources.h"

	springai::StubPoint::~StubPoint() {}
	void springai::StubPoint::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubPoint::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubPoint::SetPointId(int pointId) {
		this->pointId = pointId;
	}
	int springai::StubPoint::GetPointId() const {
		return pointId;
	}

	void springai::StubPoint::SetPosition(springai::AIFloat3 position) {
		this->position = position;
	}
	springai::AIFloat3 springai::StubPoint::GetPosition() {
		return position;
	}

	void springai::StubPoint::SetColor(springai::AIColor color) {
		this->color = color;
	}
	springai::AIColor springai::StubPoint::GetColor() {
		return color;
	}

	void springai::StubPoint::SetLabel(const char* label) {
		this->label = label;
	}
	const char* springai::StubPoint::GetLabel() {
		return label;
	}

