/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubResource.h"

#include "IncludesSources.h"

	springai::StubResource::~StubResource() {}
	void springai::StubResource::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubResource::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubResource::SetResourceId(int resourceId) {
		this->resourceId = resourceId;
	}
	int springai::StubResource::GetResourceId() const {
		return resourceId;
	}

	void springai::StubResource::SetName(const char* name) {
		this->name = name;
	}
	const char* springai::StubResource::GetName() {
		return name;
	}

	void springai::StubResource::SetOptimum(float optimum) {
		this->optimum = optimum;
	}
	float springai::StubResource::GetOptimum() {
		return optimum;
	}

