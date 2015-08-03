/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractResource.h"

#include "IncludesSources.h"

	springai::AbstractResource::~AbstractResource() {}
	int springai::AbstractResource::CompareTo(const Resource& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Resource*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetResourceId() < other.GetResourceId()) return BEFORE;
		if (this->GetResourceId() > other.GetResourceId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractResource::Equals(const Resource& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetResourceId() != other.GetResourceId()) return false;
	return true;
}

int springai::AbstractResource::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetResourceId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractResource::ToString() {

	std::string internal_ret = "Resource";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "resourceId=%i, ", this->GetResourceId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

