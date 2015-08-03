/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractFeatureDef.h"

#include "IncludesSources.h"

	springai::AbstractFeatureDef::~AbstractFeatureDef() {}
	int springai::AbstractFeatureDef::CompareTo(const FeatureDef& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((FeatureDef*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetFeatureDefId() < other.GetFeatureDefId()) return BEFORE;
		if (this->GetFeatureDefId() > other.GetFeatureDefId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractFeatureDef::Equals(const FeatureDef& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetFeatureDefId() != other.GetFeatureDefId()) return false;
	return true;
}

int springai::AbstractFeatureDef::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetFeatureDefId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractFeatureDef::ToString() {

	std::string internal_ret = "FeatureDef";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "featureDefId=%i, ", this->GetFeatureDefId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

