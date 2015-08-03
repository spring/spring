/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappVersion.h"

#include "IncludesSources.h"

	springai::WrappVersion::WrappVersion(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappVersion::~WrappVersion() {

	}

	int springai::WrappVersion::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappVersion::Version* springai::WrappVersion::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Version* internal_ret = NULL;
		internal_ret = new springai::WrappVersion(skirmishAIId);
		return internal_ret;
	}


	const char* springai::WrappVersion::GetMajor() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getMajor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetMinor() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getMinor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetPatchset() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getPatchset(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetCommits() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getCommits(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetHash() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getHash(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetBranch() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getBranch(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetAdditional() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getAdditional(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetBuildTime() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getBuildTime(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappVersion::IsRelease() {

		bool internal_ret_int;

		internal_ret_int = bridged_Engine_Version_isRelease(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetNormal() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getNormal(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetSync() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getSync(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappVersion::GetFull() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Engine_Version_getFull(this->GetSkirmishAIId());
		return internal_ret_int;
	}
