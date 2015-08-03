/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappFile.h"

#include "IncludesSources.h"

	springai::WrappFile::WrappFile(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappFile::~WrappFile() {

	}

	int springai::WrappFile::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappFile::File* springai::WrappFile::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::File* internal_ret = NULL;
		internal_ret = new springai::WrappFile(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappFile::GetSize(const char* fileName) {

		int internal_ret_int;

		internal_ret_int = bridged_File_getSize(this->GetSkirmishAIId(), fileName);
		return internal_ret_int;
	}

	bool springai::WrappFile::GetContent(const char* fileName, void* buffer, int bufferLen) {

		bool internal_ret_int;

		internal_ret_int = bridged_File_getContent(this->GetSkirmishAIId(), fileName, buffer, bufferLen);
		return internal_ret_int;
	}
