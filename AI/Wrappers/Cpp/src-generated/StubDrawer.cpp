/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubDrawer.h"

#include "IncludesSources.h"

	springai::StubDrawer::~StubDrawer() {}
	void springai::StubDrawer::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubDrawer::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubDrawer::AddNotification(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha) {
	}

	void springai::StubDrawer::AddPoint(const springai::AIFloat3& pos, const char* label) {
	}

	void springai::StubDrawer::DeletePointsAndLines(const springai::AIFloat3& pos) {
	}

	void springai::StubDrawer::AddLine(const springai::AIFloat3& posFrom, const springai::AIFloat3& posTo) {
	}

	void springai::StubDrawer::DrawUnit(UnitDef* toDrawUnitDef, const springai::AIFloat3& pos, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing) {
	}

	int springai::StubDrawer::TraceRay(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags) {
		return 0;
	}

	int springai::StubDrawer::TraceRayFeature(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags) {
		return 0;
	}

	void springai::StubDrawer::SetFigure(springai::Figure* figure) {
		this->figure = figure;
	}
	springai::Figure* springai::StubDrawer::GetFigure() {
		return figure;
	}

	void springai::StubDrawer::SetPathDrawer(springai::PathDrawer* pathDrawer) {
		this->pathDrawer = pathDrawer;
	}
	springai::PathDrawer* springai::StubDrawer::GetPathDrawer() {
		return pathDrawer;
	}

