/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappResource.h"

#include "IncludesSources.h"

	springai::WrappResource::WrappResource(int skirmishAIId, int resourceId) {

		this->skirmishAIId = skirmishAIId;
		this->resourceId = resourceId;
	}

	springai::WrappResource::~WrappResource() {

	}

	int springai::WrappResource::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappResource::GetResourceId() const {

		return resourceId;
	}

	springai::WrappResource::Resource* springai::WrappResource::GetInstance(int skirmishAIId, int resourceId) {

		if (resourceId < 0) {
			return NULL;
		}

		springai::Resource* internal_ret = NULL;
		internal_ret = new springai::WrappResource(skirmishAIId, resourceId);
		return internal_ret;
	}


	const char* springai::WrappResource::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Resource_getName(this->GetSkirmishAIId(), this->GetResourceId());
		return internal_ret_int;
	}

	float springai::WrappResource::GetOptimum() {

		float internal_ret_int;

		internal_ret_int = bridged_Resource_getOptimum(this->GetSkirmishAIId(), this->GetResourceId());
		return internal_ret_int;
	}
