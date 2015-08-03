/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubFigure.h"

#include "IncludesSources.h"

	springai::StubFigure::~StubFigure() {}
	void springai::StubFigure::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubFigure::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	int springai::StubFigure::DrawSpline(const springai::AIFloat3& pos1, const springai::AIFloat3& pos2, const springai::AIFloat3& pos3, const springai::AIFloat3& pos4, float width, bool arrow, int lifeTime, int figureGroupId) {
		return 0;
	}

	int springai::StubFigure::DrawLine(const springai::AIFloat3& pos1, const springai::AIFloat3& pos2, float width, bool arrow, int lifeTime, int figureGroupId) {
		return 0;
	}

	void springai::StubFigure::SetColor(int figureGroupId, const springai::AIColor& color, short alpha) {
	}

	void springai::StubFigure::Remove(int figureGroupId) {
	}

