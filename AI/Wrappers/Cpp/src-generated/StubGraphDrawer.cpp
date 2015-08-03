/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubGraphDrawer.h"

#include "IncludesSources.h"

	springai::StubGraphDrawer::~StubGraphDrawer() {}
	void springai::StubGraphDrawer::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubGraphDrawer::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubGraphDrawer::SetEnabled(bool isEnabled) {
		this->isEnabled = isEnabled;
	}
	bool springai::StubGraphDrawer::IsEnabled() {
		return isEnabled;
	}

	void springai::StubGraphDrawer::SetPosition(float x, float y) {
	}

	void springai::StubGraphDrawer::SetSize(float w, float h) {
	}

	void springai::StubGraphDrawer::SetGraphLine(springai::GraphLine* graphLine) {
		this->graphLine = graphLine;
	}
	springai::GraphLine* springai::StubGraphDrawer::GetGraphLine() {
		return graphLine;
	}

