/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubPathDrawer.h"

#include "IncludesSources.h"

	springai::StubPathDrawer::~StubPathDrawer() {}
	void springai::StubPathDrawer::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubPathDrawer::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubPathDrawer::Start(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha) {
	}

	void springai::StubPathDrawer::Finish(bool iAmUseless) {
	}

	void springai::StubPathDrawer::DrawLine(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) {
	}

	void springai::StubPathDrawer::DrawLineAndCommandIcon(Command* cmd, const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) {
	}

	void springai::StubPathDrawer::DrawIcon(Command* cmd) {
	}

	void springai::StubPathDrawer::Suspend(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) {
	}

	void springai::StubPathDrawer::Restart(bool sameColor) {
	}

