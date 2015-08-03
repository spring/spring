/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubRoots.h"

#include "IncludesSources.h"

	springai::StubRoots::~StubRoots() {}
	void springai::StubRoots::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubRoots::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubRoots::SetSize(int size) {
		this->size = size;
	}
	int springai::StubRoots::GetSize() {
		return size;
	}

	bool springai::StubRoots::GetDir(char* path, int path_sizeMax, int dirIndex) {
		return false;
	}

	bool springai::StubRoots::LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {
		return false;
	}

	char* springai::StubRoots::AllocatePath(const char* const relPath, bool writeable, bool create, bool dir) {
		return 0;
	}

