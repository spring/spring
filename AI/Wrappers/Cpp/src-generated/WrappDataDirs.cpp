/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappDataDirs.h"

#include "IncludesSources.h"

	springai::WrappDataDirs::WrappDataDirs(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappDataDirs::~WrappDataDirs() {

	}

	int springai::WrappDataDirs::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappDataDirs::DataDirs* springai::WrappDataDirs::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::DataDirs* internal_ret = NULL;
		internal_ret = new springai::WrappDataDirs(skirmishAIId);
		return internal_ret;
	}


	char springai::WrappDataDirs::GetPathSeparator() {

		char internal_ret_int;

		internal_ret_int = bridged_DataDirs_getPathSeparator(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappDataDirs::GetConfigDir() {

		const char* internal_ret_int;

		internal_ret_int = bridged_DataDirs_getConfigDir(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappDataDirs::GetWriteableDir() {

		const char* internal_ret_int;

		internal_ret_int = bridged_DataDirs_getWriteableDir(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappDataDirs::LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

		bool internal_ret_int;

		internal_ret_int = bridged_DataDirs_locatePath(this->GetSkirmishAIId(), path, path_sizeMax, relPath, writeable, create, dir, common);
		return internal_ret_int;
	}

	char* springai::WrappDataDirs::AllocatePath(const char* const relPath, bool writeable, bool create, bool dir, bool common) {

		char* internal_ret_int;

		internal_ret_int = bridged_DataDirs_allocatePath(this->GetSkirmishAIId(), relPath, writeable, create, dir, common);
		return internal_ret_int;
	}

	springai::Roots* springai::WrappDataDirs::GetRoots() {

		Roots* internal_ret_int_out;

		internal_ret_int_out = springai::WrappRoots::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}
