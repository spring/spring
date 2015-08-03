/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubLog.h"

#include "IncludesSources.h"

	springai::StubLog::~StubLog() {}
	void springai::StubLog::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubLog::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubLog::DoLog(const char* const msg) {
	}

	void springai::StubLog::Exception(const char* const msg, int severety, bool die) {
	}

