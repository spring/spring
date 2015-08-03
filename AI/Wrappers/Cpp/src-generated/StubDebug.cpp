/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubDebug.h"

#include "IncludesSources.h"

	springai::StubDebug::~StubDebug() {}
	void springai::StubDebug::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubDebug::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	int springai::StubDebug::AddOverlayTexture(const float* texData, int w, int h) {
		return 0;
	}

	void springai::StubDebug::SetGraphDrawer(springai::GraphDrawer* graphDrawer) {
		this->graphDrawer = graphDrawer;
	}
	springai::GraphDrawer* springai::StubDebug::GetGraphDrawer() {
		return graphDrawer;
	}

