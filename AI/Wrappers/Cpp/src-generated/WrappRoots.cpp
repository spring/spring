/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappRoots.h"

#include "IncludesSources.h"

	springai::WrappRoots::WrappRoots(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappRoots::~WrappRoots() {

	}

	int springai::WrappRoots::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappRoots::Roots* springai::WrappRoots::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Roots* internal_ret = NULL;
		internal_ret = new springai::WrappRoots(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappRoots::GetSize() {

		int internal_ret_int;

		internal_ret_int = bridged_DataDirs_Roots_getSize(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappRoots::GetDir(char* path, int path_sizeMax, int dirIndex) {

		bool internal_ret_int;

		internal_ret_int = bridged_DataDirs_Roots_getDir(this->GetSkirmishAIId(), path, path_sizeMax, dirIndex);
		return internal_ret_int;
	}

	bool springai::WrappRoots::LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {

		bool internal_ret_int;

		internal_ret_int = bridged_DataDirs_Roots_locatePath(this->GetSkirmishAIId(), path, path_sizeMax, relPath, writeable, create, dir);
		return internal_ret_int;
	}

	char* springai::WrappRoots::AllocatePath(const char* const relPath, bool writeable, bool create, bool dir) {

		char* internal_ret_int;

		internal_ret_int = bridged_DataDirs_Roots_allocatePath(this->GetSkirmishAIId(), relPath, writeable, create, dir);
		return internal_ret_int;
	}
