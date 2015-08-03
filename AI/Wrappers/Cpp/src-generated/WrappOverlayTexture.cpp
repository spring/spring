/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappOverlayTexture.h"

#include "IncludesSources.h"

	springai::WrappOverlayTexture::WrappOverlayTexture(int skirmishAIId, int overlayTextureId) {

		this->skirmishAIId = skirmishAIId;
		this->overlayTextureId = overlayTextureId;
	}

	springai::WrappOverlayTexture::~WrappOverlayTexture() {

	}

	int springai::WrappOverlayTexture::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappOverlayTexture::GetOverlayTextureId() const {

		return overlayTextureId;
	}

	springai::WrappOverlayTexture::OverlayTexture* springai::WrappOverlayTexture::GetInstance(int skirmishAIId, int overlayTextureId) {

		if (overlayTextureId < 0) {
			return NULL;
		}

		springai::OverlayTexture* internal_ret = NULL;
		internal_ret = new springai::WrappOverlayTexture(skirmishAIId, overlayTextureId);
		return internal_ret;
	}


	void springai::WrappOverlayTexture::Update(const float* texData, int x, int y, int w, int h) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_OverlayTexture_update(this->GetSkirmishAIId(), this->GetOverlayTextureId(), texData, x, y, w, h);
	if (internal_ret_int != 0) {
		throw CallbackAIException("update", internal_ret_int);
	}

	}

	void springai::WrappOverlayTexture::Remove() {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_OverlayTexture_remove(this->GetSkirmishAIId(), this->GetOverlayTextureId());
	if (internal_ret_int != 0) {
		throw CallbackAIException("remove", internal_ret_int);
	}

	}

	void springai::WrappOverlayTexture::SetPosition(float x, float y) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_OverlayTexture_setPosition(this->GetSkirmishAIId(), this->GetOverlayTextureId(), x, y);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setPosition", internal_ret_int);
	}

	}

	void springai::WrappOverlayTexture::SetSize(float w, float h) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_OverlayTexture_setSize(this->GetSkirmishAIId(), this->GetOverlayTextureId(), w, h);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setSize", internal_ret_int);
	}

	}

	void springai::WrappOverlayTexture::SetLabel(const char* label) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_OverlayTexture_setLabel(this->GetSkirmishAIId(), this->GetOverlayTextureId(), label);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setLabel", internal_ret_int);
	}

	}
