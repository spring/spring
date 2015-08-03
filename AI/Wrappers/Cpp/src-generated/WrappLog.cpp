/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappLog.h"

#include "IncludesSources.h"

	springai::WrappLog::WrappLog(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappLog::~WrappLog() {

	}

	int springai::WrappLog::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappLog::Log* springai::WrappLog::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Log* internal_ret = NULL;
		internal_ret = new springai::WrappLog(skirmishAIId);
		return internal_ret;
	}


	void springai::WrappLog::DoLog(const char* const msg) {

		bridged_Log_log(this->GetSkirmishAIId(), msg);
	}

	void springai::WrappLog::Exception(const char* const msg, int severety, bool die) {

		bridged_Log_exception(this->GetSkirmishAIId(), msg, severety, die);
	}
