/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubLine.h"

#include "IncludesSources.h"

	springai::StubLine::~StubLine() {}
	void springai::StubLine::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubLine::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubLine::SetLineId(int lineId) {
		this->lineId = lineId;
	}
	int springai::StubLine::GetLineId() const {
		return lineId;
	}

	void springai::StubLine::SetFirstPosition(springai::AIFloat3 firstPosition) {
		this->firstPosition = firstPosition;
	}
	springai::AIFloat3 springai::StubLine::GetFirstPosition() {
		return firstPosition;
	}

	void springai::StubLine::SetSecondPosition(springai::AIFloat3 secondPosition) {
		this->secondPosition = secondPosition;
	}
	springai::AIFloat3 springai::StubLine::GetSecondPosition() {
		return secondPosition;
	}

	void springai::StubLine::SetColor(springai::AIColor color) {
		this->color = color;
	}
	springai::AIColor springai::StubLine::GetColor() {
		return color;
	}

