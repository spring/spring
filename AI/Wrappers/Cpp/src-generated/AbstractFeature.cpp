/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractFeature.h"

#include "IncludesSources.h"

	springai::AbstractFeature::~AbstractFeature() {}
	int springai::AbstractFeature::CompareTo(const Feature& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Feature*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetFeatureId() < other.GetFeatureId()) return BEFORE;
		if (this->GetFeatureId() > other.GetFeatureId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractFeature::Equals(const Feature& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetFeatureId() != other.GetFeatureId()) return false;
	return true;
}

int springai::AbstractFeature::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetFeatureId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractFeature::ToString() {

	std::string internal_ret = "Feature";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "featureId=%i, ", this->GetFeatureId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

