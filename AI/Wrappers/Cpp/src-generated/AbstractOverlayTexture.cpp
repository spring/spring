/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractOverlayTexture.h"

#include "IncludesSources.h"

	springai::AbstractOverlayTexture::~AbstractOverlayTexture() {}
	int springai::AbstractOverlayTexture::CompareTo(const OverlayTexture& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((OverlayTexture*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetOverlayTextureId() < other.GetOverlayTextureId()) return BEFORE;
		if (this->GetOverlayTextureId() > other.GetOverlayTextureId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractOverlayTexture::Equals(const OverlayTexture& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetOverlayTextureId() != other.GetOverlayTextureId()) return false;
	return true;
}

int springai::AbstractOverlayTexture::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetOverlayTextureId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractOverlayTexture::ToString() {

	std::string internal_ret = "OverlayTexture";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "overlayTextureId=%i, ", this->GetOverlayTextureId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

