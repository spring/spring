/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubDataDirs.h"

#include "IncludesSources.h"

	springai::StubDataDirs::~StubDataDirs() {}
	void springai::StubDataDirs::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubDataDirs::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubDataDirs::SetPathSeparator(char pathSeparator) {
		this->pathSeparator = pathSeparator;
	}
	char springai::StubDataDirs::GetPathSeparator() {
		return pathSeparator;
	}

	void springai::StubDataDirs::SetConfigDir(const char* configDir) {
		this->configDir = configDir;
	}
	const char* springai::StubDataDirs::GetConfigDir() {
		return configDir;
	}

	void springai::StubDataDirs::SetWriteableDir(const char* writeableDir) {
		this->writeableDir = writeableDir;
	}
	const char* springai::StubDataDirs::GetWriteableDir() {
		return writeableDir;
	}

	bool springai::StubDataDirs::LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common) {
		return false;
	}

	char* springai::StubDataDirs::AllocatePath(const char* const relPath, bool writeable, bool create, bool dir, bool common) {
		return 0;
	}

	void springai::StubDataDirs::SetRoots(springai::Roots* roots) {
		this->roots = roots;
	}
	springai::Roots* springai::StubDataDirs::GetRoots() {
		return roots;
	}

